/*
 * Xray_View.cpp
 * This file implements the Xray_View class, which shows the scene as an X-ray image.
 */

#include "Xray_View.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <utility>
#include <vector>

#include <glad/glad.h>
#include <imgui.h>

#include "xray/Gvxr_Adapter.h"

namespace
{
	// The ID that ImGui uses for an OpenGL texture
	ImTextureID get_texture_id(unsigned int texture)
	{
		return static_cast<ImTextureID>(texture);
	}

	// Rectangle of the given aspect ratio (width over height) that fits in the area, centred in it
	void fit_in_area(const ImVec2& area, float aspect_ratio, ImVec2& offset, ImVec2& size)
	{
		size = ImVec2{ area.x, area.x / aspect_ratio };
		if (size.y > area.y)
			size = ImVec2{ area.y * aspect_ratio, area.y };
		offset = ImVec2{ (area.x - size.x) / 2.0f, (area.y - size.y) / 2.0f };
	}
} // namespace

// Constructors
// ------------
Xray_View::Xray_View(std::shared_ptr<const Scene> scene, bool should_check) : scene{ std::move(scene) }, is_check_requested{ should_check }
{}

// Destructor
// ----------
Xray_View::~Xray_View()
{
	if (texture != 0)
		glDeleteTextures(1, &texture);
	adapter.reset();
}

// Public Methods
// --------------
void Xray_View::draw()
{
	if (!scene)
		return;

	if (!adapter && error.empty())
	{ // the first time: the context of the window is current, so the adapter can start, and the image is computed
		initialise();
		if (error.empty())
			compute();
	}

	if (is_check_requested && is_image_valid)
	{
		is_check_requested = false;
		check_image();
	}

	draw_window();
	if (is_single_view && texture != 0)
		draw_background();
}

// Private Methods
// ---------------
void Xray_View::initialise()
{
	Xray_Beam beam;
	beam.source_position = { 0.0, 0.0, SOURCE_Z };
	beam.energy          = BEAM_ENERGY;

	Xray_Detector detector;
	detector.position         = { 0.0, 0.0, DETECTOR_Z };
	detector.up_vector        = { 0.0, 1.0, 0.0 };
	detector.width_in_pixels  = DETECTOR_WIDTH_IN_PIXELS;
	detector.height_in_pixels = DETECTOR_HEIGHT_IN_PIXELS;
	detector.pixel_size       = PIXEL_SIZE;

	auto new_adapter = std::make_unique<Gvxr_Adapter>();

	error = new_adapter->initialise(beam, detector);
	if (error.empty())
	{ // the meshes are uploaded once, in millimetres, with the transform of each object applied
		for (const Xray_Mesh& mesh : scene->make_xray_meshes())
		{
			error = new_adapter->add_mesh(mesh);
			if (!error.empty())
				break;
		}
	}

	if (!error.empty())
		std::cerr << "[ERROR::XRAY_VIEW::initialise] " << error << std::endl;
	adapter = std::move(new_adapter); // kept even after a failure, so that the initialisation is not tried again every frame
}

void Xray_View::compute()
{
	if (!adapter || !error.empty())
		return;

	const auto start = std::chrono::steady_clock::now();
	error            = adapter->compute_image(image);
	compute_time_ms  = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

	if (!error.empty())
	{
		is_image_valid = false;
		std::cerr << "[ERROR::XRAY_VIEW::compute] " << error << std::endl;
		return;
	}

	is_image_valid = image.width > 0 && image.height > 0 && image.pixels.size() == static_cast<std::size_t>(image.width) * image.height;
	if (!is_image_valid)
	{
		error = "The image has " + std::to_string(image.pixels.size()) + " pixels for " + std::to_string(image.width) + " by " +
			std::to_string(image.height);
		std::cerr << "[ERROR::XRAY_VIEW::compute] " << error << std::endl;
		return;
	}

	// the library numbers the columns as seen from the source, looking along +z, where +x is on the left. The image is mirrored so
	// that it is seen from the detector, looking along -z, as the 3D camera of the scene looks: +x is on the right and +y is up
	for (unsigned int row = 0; row < image.height; ++row)
	{
		const auto first = image.pixels.begin() + static_cast<std::ptrdiff_t>(row) * image.width;
		std::reverse(first, first + image.width);
	}

	reference        = Xray_Display::find_reference_value(image);
	is_texture_dirty = true;
	std::cout << "[INFO::XRAY_VIEW::compute] X-ray image of " << image.width << " by " << image.height << " pixels computed in "
			  << compute_time_ms << " ms" << std::endl;
}

void Xray_View::update_texture()
{
	if (!is_image_valid)
		return;

	if (texture == 0)
	{ // one byte per pixel, shown as grey (the red channel is copied to the three colour channels)
		glCreateTextures(GL_TEXTURE_2D, 1, &texture);
		glTextureStorage2D(texture, 1, GL_R8, static_cast<GLsizei>(image.width), static_cast<GLsizei>(image.height));
		glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		const GLint swizzle[4]{ GL_RED, GL_RED, GL_RED, GL_ONE };
		glTextureParameteriv(texture, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
	}

	const std::vector<std::uint8_t> grey = Xray_Display::convert_to_grey(image, reference, display_settings);

	// the rows are tightly packed (one byte per pixel), whatever the width
	GLint alignment{ 4 };
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTextureSubImage2D(
		texture,
		0,
		0,
		0,
		static_cast<GLsizei>(image.width),
		static_cast<GLsizei>(image.height),
		GL_RED,
		GL_UNSIGNED_BYTE,
		grey.data());
	glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);

	is_texture_dirty = false;
}

void Xray_View::draw_window()
{
	// the window is at the right of the 3D view, where it is visible from the first frame
	const ImVec2 display = ImGui::GetIO().DisplaySize;
	ImGui::SetNextWindowPos(ImVec2{ std::max(display.x - 580.0f, 0.0f), 20.0f }, ImGuiCond_FirstUseEver);
	// the window has room for the image, or only for the controls in the single view, and is resized when the view changes
	const ImVec2 size{ 560.0f, is_single_view ? 140.0f : 380.0f };
	ImGui::SetNextWindowSize(size, is_single_view != was_single_view ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
	was_single_view = is_single_view;
	ImGui::Begin("X-ray");

	ImGui::Checkbox("Single X-ray view", &is_single_view);
	ImGui::SameLine();
	if (ImGui::Button("Recompute"))
		compute();

	int mode = static_cast<int>(display_settings.mode);
	if (ImGui::Combo("Scale", &mode, "Linear\0Logarithmic\0"))
	{
		display_settings.mode = static_cast<Xray_Display_Mode>(mode);
		is_texture_dirty      = true;
	}
	if (ImGui::Checkbox("Negative", &display_settings.is_negative))
		is_texture_dirty = true;

	if (!error.empty())
	{
		ImGui::TextColored(ImVec4{ 1.0f, 0.4f, 0.4f, 1.0f }, "Error: %s", error.c_str());
	}
	else if (is_image_valid)
	{
		ImGui::Text(
			"%u x %u pixels of %.1f mm, %.0f keV, computed in %.0f ms",
			image.width,
			image.height,
			PIXEL_SIZE,
			BEAM_ENERGY,
			compute_time_ms);
	}

	if (is_texture_dirty)
		update_texture();

	if (texture != 0 && !is_single_view)
	{ // the first row of the image is the bottom of the detector, which is the bottom of the texture: the image is flipped vertically
		ImVec2 offset;
		ImVec2 size;
		fit_in_area(ImGui::GetContentRegionAvail(), static_cast<float>(image.width) / static_cast<float>(image.height), offset, size);
		ImGui::Image(get_texture_id(texture), size, ImVec2{ 0.0f, 1.0f }, ImVec2{ 1.0f, 0.0f });
	}
	else if (texture != 0)
	{
		ImGui::TextUnformatted("The image is shown behind the windows.");
	}

	ImGui::End();
}

void Xray_View::draw_background() const
{
	const ImVec2 display = ImGui::GetIO().DisplaySize;
	ImVec2       offset;
	ImVec2       size;
	fit_in_area(display, static_cast<float>(image.width) / static_cast<float>(image.height), offset, size);

	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
	draw_list->AddRectFilled(ImVec2{ 0.0f, 0.0f }, display, IM_COL32(0, 0, 0, 255));
	draw_list->AddImage(
		get_texture_id(texture),
		offset,
		ImVec2{ offset.x + size.x, offset.y + size.y },
		ImVec2{ 0.0f, 1.0f },
		ImVec2{ 1.0f, 0.0f });
}

double Xray_View::get_transmission_at(float x, float y) const
{
	// the point is in the plane z = 0 of the scene: the source projects it on the detector with a magnification of
	// (DETECTOR_Z - SOURCE_Z) / (0 - SOURCE_Z). The centre of the detector is the pixel at half of the width and of the height
	const double magnification = (DETECTOR_Z - SOURCE_Z) / -SOURCE_Z;
	const double column        = image.width / 2.0 + static_cast<double>(x) * Scene::MILLIMETRES_PER_UNIT * magnification / PIXEL_SIZE;
	const double row           = image.height / 2.0 + static_cast<double>(y) * Scene::MILLIMETRES_PER_UNIT * magnification / PIXEL_SIZE;
	const int    centre_x      = static_cast<int>(std::floor(column));
	const int    centre_y      = static_cast<int>(std::floor(row));
	if (centre_x < 1 || centre_y < 1 || centre_x + 1 >= static_cast<int>(image.width) || centre_y + 1 >= static_cast<int>(image.height))
		return -1.0;

	double sum{ 0.0 };
	for (int j = centre_y - 1; j <= centre_y + 1; ++j)
		for (int i = centre_x - 1; i <= centre_x + 1; ++i) sum += image.pixels[static_cast<std::size_t>(j) * image.width + i];
	return sum / 9.0 / reference;
}

void Xray_View::check_image() const
{
	const auto fail = [](const std::string& reason) {
		std::cerr << "[ERROR::XRAY_VIEW::check_image] " << reason << std::endl;
	};

	if (!(reference > 0.0f))
	{
		fail("The image has no beam");
		return;
	}
	if (!std::all_of(image.pixels.begin(), image.pixels.end(), [](float value) { return std::isfinite(value) && value >= 0.0f; }))
	{
		fail("The image has negative or non-finite values");
		return;
	}

	// the corners of the detector are far from the objects: the beam there is nearly unattenuated
	double corner_transmission{ 1.0 };
	for (const std::size_t index : { std::size_t{ 0 },
									 std::size_t{ image.width - 1 },
									 std::size_t{ image.pixels.size() - image.width },
									 std::size_t{ image.pixels.size() - 1 } })
		corner_transmission = std::min(corner_transmission, static_cast<double>(image.pixels[index]) / reference);
	std::cout << "[INFO::XRAY_VIEW::check_image] lowest transmission in the corners: " << corner_transmission << std::endl;
	if (corner_transmission < 0.9)
	{
		fail("The corners of the image are attenuated (transmission " + std::to_string(corner_transmission) + ")");
		return;
	}

	// each object attenuates the beam where its centre projects (the objects are in the plane z = 0 in the default scene)
	double water{ -1.0 };
	double iron{ -1.0 };
	for (const Scene_Object& object : scene->get_objects())
	{
		const double transmission = get_transmission_at(object.position[0], object.position[1]);
		std::cout << "[INFO::XRAY_VIEW::check_image] transmission behind '" << object.name << "': " << transmission << std::endl;
		if (!(transmission >= 0.0 && transmission < 0.9))
		{
			fail("The beam is not attenuated behind '" + object.name + "' (transmission " + std::to_string(transmission) + ")");
			return;
		}
		if (object.name == "Water Sphere")
			water = transmission;
		if (object.name == "Iron Sphere")
			iron = transmission;
	}
	if (water >= 0.0 && iron >= 0.0 && !(iron < water))
	{
		fail(
			"The iron sphere does not attenuate more than the water sphere (" + std::to_string(iron) + " and " + std::to_string(water) +
			")");
		return;
	}

	std::cout << "[SUCCESS::XRAY_VIEW::check_image] The X-ray image of the scene is valid and each object attenuates the beam" << std::endl;
}
