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

	// The images that are counted are the ones of the last seconds (an image takes more than a second, so a shorter period would count
	// none)
	constexpr int IMAGE_COUNT_SECONDS{ 5 };

	// Milliseconds since the time point
	double get_milliseconds_since(std::chrono::steady_clock::time_point start)
	{
		return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
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
		apply_setup();
	}

	if (is_check_requested && is_image_valid)
	{
		is_check_requested = false;
		check_image();
	}

	// the setup is drawn first, so that the image that it changes is the one that the X-ray window shows in the same frame
	draw_setup_window();
	draw_window();
	if (is_single_view && texture != 0)
		draw_background();
}

// Private Methods
// ---------------
void Xray_View::apply_setup()
{
	setup.clamp();
	error.clear();
	is_setup_dirty = false;

	const auto apply_start = std::chrono::steady_clock::now();

	// the old adapter is destroyed first: the library keeps one graphics context, which the new adapter registers
	adapter.reset();
	adapter_reset_ms        = get_milliseconds_since(apply_start);
	const auto create_start = std::chrono::steady_clock::now();
	auto       new_adapter  = std::make_unique<Gvxr_Adapter>();
	adapter_create_ms       = get_milliseconds_since(create_start);

	error = new_adapter->initialise(setup.make_beam(), setup.make_detector());
	if (error.empty())
	{ // the meshes are uploaded in millimetres, with the transform of each object applied
		const auto                   scene_start = std::chrono::steady_clock::now();
		const std::vector<Xray_Mesh> meshes      = scene->make_xray_meshes();
		scene_ms                                 = get_milliseconds_since(scene_start);
		for (const Xray_Mesh& mesh : meshes)
		{
			error = new_adapter->add_mesh(mesh);
			if (!error.empty())
				break;
		}
	}

	adapter     = std::move(new_adapter); // kept even after a failure, so that the initialisation is not tried again every frame
	image_setup = setup;

	if (!error.empty())
	{
		is_image_valid = false;
		std::cerr << "[ERROR::XRAY_VIEW::apply_setup] " << error << std::endl;
		return;
	}
	compute();

	apply_ms = get_milliseconds_since(apply_start);
	if (!error.empty())
		return;
	image_times.push_back(std::chrono::steady_clock::now());
	std::cout << "[INFO::XRAY_VIEW::apply_setup] " << image.width << " by " << image.height << " pixels, " << timings.mesh_count
			  << " meshes: total " << apply_ms << " ms; destroy the adapter " << adapter_reset_ms << ", create the adapter "
			  << adapter_create_ms << ", initialise " << timings.initialise_ms << " (GLEW " << timings.glew_ms << ", renderer "
			  << timings.renderer_initialise_ms << ") + scene " << scene_ms << ", add meshes " << timings.add_meshes_ms << ", compute call "
			  << compute_time_ms << " (computation " << timings.computation_ms << ", read-back " << timings.read_back_ms << "), mirror "
			  << mirror_ms << "; state guard " << timings.guard_ms << " ms in all the calls; images in the last " << IMAGE_COUNT_SECONDS
			  << " seconds: " << get_images_in_last_seconds() << std::endl;
}

void Xray_View::compute()
{
	if (!adapter || !error.empty())
		return;

	const auto start = std::chrono::steady_clock::now();
	error            = adapter->compute_image(image);
	compute_time_ms  = get_milliseconds_since(start);
	timings          = adapter->get_timings();

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
	const auto mirror_start = std::chrono::steady_clock::now();
	for (unsigned int row = 0; row < image.height; ++row)
	{
		const auto first = image.pixels.begin() + static_cast<std::ptrdiff_t>(row) * image.width;
		std::reverse(first, first + image.width);
	}

	reference        = Xray_Display::find_reference_value(image);
	mirror_ms        = get_milliseconds_since(mirror_start);
	is_texture_dirty = true;
	std::cout << "[INFO::XRAY_VIEW::compute] X-ray image of " << image.width << " by " << image.height << " pixels computed in "
			  << compute_time_ms << " ms" << std::endl;
}

void Xray_View::update_texture()
{
	if (!is_image_valid)
		return;

	if (texture != 0 && (texture_width != image.width || texture_height != image.height))
	{ // the storage of a texture cannot change its size: it is created again
		glDeleteTextures(1, &texture);
		texture = 0;
	}

	const auto creation_start = std::chrono::steady_clock::now();
	if (texture == 0)
	{ // one byte per pixel, shown as grey (the red channel is copied to the three colour channels)
		texture_width  = image.width;
		texture_height = image.height;
		glCreateTextures(GL_TEXTURE_2D, 1, &texture);
		glTextureStorage2D(texture, 1, GL_R8, static_cast<GLsizei>(image.width), static_cast<GLsizei>(image.height));
		glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		const GLint swizzle[4]{ GL_RED, GL_RED, GL_RED, GL_ONE };
		glTextureParameteriv(texture, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
	}

	const double creation_ms = get_milliseconds_since(creation_start);

	const auto                      grey_start = std::chrono::steady_clock::now();
	const std::vector<std::uint8_t> grey       = Xray_Display::convert_to_grey(image, reference, display_settings);
	grey_ms                                    = get_milliseconds_since(grey_start);

	// the rows are tightly packed (one byte per pixel), whatever the width
	const auto copy_start = std::chrono::steady_clock::now();
	GLint      alignment{ 4 };
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

	// the driver may finish the transfer to the GPU later, so this is the time that the application waits
	upload_ms        = creation_ms + get_milliseconds_since(copy_start);
	is_texture_dirty = false;
	std::cout << "[INFO::XRAY_VIEW::update_texture] grey conversion " << grey_ms << " ms, texture upload " << upload_ms << " ms"
			  << std::endl;
}

unsigned int Xray_View::get_images_in_last_seconds()
{
	const auto now = std::chrono::steady_clock::now();
	std::erase_if(image_times, [now](std::chrono::steady_clock::time_point time) {
		return now - time > std::chrono::seconds{ IMAGE_COUNT_SECONDS };
	});

	const auto count            = static_cast<unsigned int>(image_times.size());
	most_images_in_last_seconds = std::max(most_images_in_last_seconds, count);
	return count;
}

void Xray_View::draw_setup_window()
{
	// the window is under the X-ray window, at the right of the 3D view
	const ImVec2 display = ImGui::GetIO().DisplaySize;
	ImGui::SetNextWindowPos(ImVec2{ std::max(display.x - 580.0f, 0.0f), 410.0f }, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2{ 560.0f, 360.0f }, ImGuiCond_FirstUseEver);
	ImGui::Begin("X-ray setup");

	// the values are clamped by the sliders (also when a value is typed), so that they are always valid
	bool      is_changed{ false };
	const int clamp_flag = ImGuiSliderFlags_AlwaysClamp;
	is_changed |= ImGui::SliderFloat("Energy (keV)", &setup.energy, Xray_Setup::MIN_ENERGY, Xray_Setup::MAX_ENERGY, "%.0f", clamp_flag);
	is_changed |= ImGui::SliderFloat(
		"Source distance (cm)",
		&setup.source_distance,
		Xray_Setup::MIN_SOURCE_DISTANCE,
		Xray_Setup::MAX_SOURCE_DISTANCE,
		"%.0f",
		clamp_flag);
	is_changed |= ImGui::SliderFloat(
		"Detector distance (cm)",
		&setup.detector_distance,
		Xray_Setup::MIN_DETECTOR_DISTANCE,
		Xray_Setup::MAX_DETECTOR_DISTANCE,
		"%.0f",
		clamp_flag);
	is_changed |= ImGui::SliderFloat(
		"Pixel size (mm)",
		&setup.pixel_size,
		Xray_Setup::MIN_PIXEL_SIZE,
		Xray_Setup::MAX_PIXEL_SIZE,
		"%.1f",
		clamp_flag);

	int width  = static_cast<int>(setup.width_in_pixels);
	int height = static_cast<int>(setup.height_in_pixels);
	if (ImGui::SliderInt("Detector width (pixels)", &width, Xray_Setup::MIN_PIXELS, Xray_Setup::MAX_PIXELS, "%d", clamp_flag))
	{
		setup.width_in_pixels = static_cast<unsigned int>(width);
		is_changed            = true;
	}
	if (ImGui::SliderInt("Detector height (pixels)", &height, Xray_Setup::MIN_PIXELS, Xray_Setup::MAX_PIXELS, "%d", clamp_flag))
	{
		setup.height_in_pixels = static_cast<unsigned int>(height);
		is_changed             = true;
	}

	ImGui::Text(
		"Magnification %.2f, detector %.0f x %.0f mm",
		setup.get_magnification(),
		setup.width_in_pixels * setup.pixel_size,
		setup.height_in_pixels * setup.pixel_size);

	ImGui::Checkbox("Recompute on change", &is_auto_recompute);
	ImGui::SameLine();
	const bool is_apply_pressed = ImGui::Button("Apply");
	ImGui::SameLine();
	if (ImGui::Button("Reset"))
	{
		setup      = Xray_Setup{};
		is_changed = true;
	}

	if (is_changed)
		is_setup_dirty = true;
	if (is_setup_dirty && (is_auto_recompute || is_apply_pressed))
		apply_setup();

	// the times of the last image, in milliseconds
	ImGui::Separator();
	if (is_image_valid)
	{
		ImGui::Text("Last image: %.1f ms in all, %u meshes", apply_ms, timings.mesh_count);
		ImGui::Text(
			"Destroy %.1f, create %.1f, initialise %.1f (GLEW %.1f, renderer %.1f), scene %.1f, add meshes %.1f",
			adapter_reset_ms,
			adapter_create_ms,
			timings.initialise_ms,
			timings.glew_ms,
			timings.renderer_initialise_ms,
			scene_ms,
			timings.add_meshes_ms);
		ImGui::Text(
			"Computation %.1f, read-back %.1f, mirror %.1f, grey %.1f, texture %.1f",
			timings.computation_ms,
			timings.read_back_ms,
			mirror_ms,
			grey_ms,
			upload_ms);
		ImGui::Text("Compute call %.1f, state guard %.1f in all the calls", compute_time_ms, timings.guard_ms);
	}
	ImGui::Text(
		"Images computed in the last %d seconds: %u (most: %u), frames per second: %.0f",
		IMAGE_COUNT_SECONDS,
		get_images_in_last_seconds(),
		most_images_in_last_seconds,
		ImGui::GetIO().Framerate);

	ImGui::End();
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
	{ // after an error, the adapter is created again
		if (error.empty() && adapter)
			compute();
		else
			apply_setup();
	}

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
			image_setup.pixel_size,
			image_setup.energy,
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
	// the point is in the plane z = 0 of the scene: the source projects it on the detector with the magnification of the setup of the
	// image. The centre of the detector is the pixel at half of the width and of the height
	const double scale    = static_cast<double>(Scene::MILLIMETRES_PER_UNIT) * image_setup.get_magnification() / image_setup.pixel_size;
	const double column   = image.width / 2.0 + static_cast<double>(x) * scale;
	const double row      = image.height / 2.0 + static_cast<double>(y) * scale;
	const int    centre_x = static_cast<int>(std::floor(column));
	const int    centre_y = static_cast<int>(std::floor(row));
	if (centre_x < 1 || centre_y < 1 || centre_x + 1 >= static_cast<int>(image.width) || centre_y + 1 >= static_cast<int>(image.height))
		return -1.0;

	double sum{ 0.0 };
	for (int j = centre_y - 1; j <= centre_y + 1; ++j)
		for (int i = centre_x - 1; i <= centre_x + 1; ++i) sum += image.pixels[static_cast<std::size_t>(j) * image.width + i];
	return sum / 9.0 / reference;
}

std::string Xray_View::check_current_image(double& water_transmission, double& iron_transmission) const
{
	water_transmission = -1.0;
	iron_transmission  = -1.0;

	if (!error.empty() || !is_image_valid)
		return "There is no valid image: " + error;
	if (!(reference > 0.0f))
		return "The image has no beam";
	if (!std::all_of(image.pixels.begin(), image.pixels.end(), [](float value) { return std::isfinite(value) && value >= 0.0f; }))
		return "The image has negative or non-finite values";

	// the corners of the detector are far from the objects: the beam there is nearly unattenuated
	double corner_transmission{ 1.0 };
	for (const std::size_t index : { std::size_t{ 0 },
									 std::size_t{ image.width - 1 },
									 std::size_t{ image.pixels.size() - image.width },
									 std::size_t{ image.pixels.size() - 1 } })
		corner_transmission = std::min(corner_transmission, static_cast<double>(image.pixels[index]) / reference);
	std::cout << "[INFO::XRAY_VIEW::check_image] lowest transmission in the corners: " << corner_transmission << std::endl;
	if (corner_transmission < 0.9)
		return "The corners of the image are attenuated (transmission " + std::to_string(corner_transmission) + ")";

	// each object attenuates the beam where its centre projects (the objects are in the plane z = 0 in the default scene)
	for (const Scene_Object& object : scene->get_objects())
	{
		const double transmission = get_transmission_at(object.position[0], object.position[1]);
		std::cout << "[INFO::XRAY_VIEW::check_image] transmission behind '" << object.name << "': " << transmission << std::endl;
		if (!(transmission >= 0.0 && transmission < 0.9))
			return "The beam is not attenuated behind '" + object.name + "' (transmission " + std::to_string(transmission) + ")";
		if (object.name == "Water Sphere")
			water_transmission = transmission;
		if (object.name == "Iron Sphere")
			iron_transmission = transmission;
	}
	return "";
}

void Xray_View::check_image()
{
	const auto fail = [](const std::string& reason) {
		std::cerr << "[ERROR::XRAY_VIEW::check_image] " << reason << std::endl;
	};

	// the first image, with the setup of the start
	double      water{ -1.0 };
	double      iron{ -1.0 };
	std::string reason = check_current_image(water, iron);
	if (!reason.empty())
	{
		fail(reason);
		return;
	}
	if (water >= 0.0 && iron >= 0.0 && !(iron < water))
	{
		fail(
			"The iron sphere does not attenuate more than the water sphere (" + std::to_string(iron) + " and " + std::to_string(water) +
			")");
		return;
	}

	// a lower energy attenuates more (the mass attenuation coefficient of water is 0.268 cm2/g at 40 keV and 0.184 cm2/g at 80 keV, so
	// the 8 cm of the water sphere transmit 12 percent instead of 23 percent)
	const Xray_Setup original = setup;
	setup.energy              = 40.0f;
	apply_setup();
	double water_at_40_kev{ -1.0 };
	double iron_at_40_kev{ -1.0 };
	reason = check_current_image(water_at_40_kev, iron_at_40_kev);
	std::cout << "[INFO::XRAY_VIEW::check_image] transmission behind the water sphere: " << water << " at 80 keV, " << water_at_40_kev
			  << " at 40 keV" << std::endl;
	if (!reason.empty())
	{
		fail("At 40 keV: " + reason);
		return;
	}
	if (water >= 0.0 && !(water_at_40_kev < 0.8 * water))
	{
		fail(
			"The water sphere does not attenuate more at 40 keV (" + std::to_string(water_at_40_kev) + ") than at 80 keV (" +
			std::to_string(water) + ")");
		return;
	}

	// another geometry and another detector: the image has the new size, and the shadows are where the new magnification puts them
	setup                   = original;
	setup.source_distance   = 150.0f;
	setup.detector_distance = 100.0f;
	setup.pixel_size        = 4.0f;
	setup.width_in_pixels   = 128;
	setup.height_in_pixels  = 64;
	apply_setup();
	reason = check_current_image(water, iron);
	if (reason.empty() && (image.width != 128 || image.height != 64))
		reason = "The image is " + std::to_string(image.width) + " by " + std::to_string(image.height) + " pixels, not 128 by 64";
	if (reason.empty())
	{ // the texture is created again with the new size, without an error of OpenGL
		while (glGetError() != GL_NO_ERROR)
		{}
		update_texture();
		if (glGetError() != GL_NO_ERROR || texture == 0 || texture_width != 128 || texture_height != 64)
			reason = "The texture of the image was not created again with the new size";
	}
	if (reason.empty() && water >= 0.0 && iron >= 0.0 && !(iron < water))
		reason = "The iron sphere does not attenuate more than the water sphere";
	if (!reason.empty())
	{
		fail("With another geometry and detector: " + reason);
		return;
	}

	// the setup of the start again (it also checks that the image comes back)
	setup = original;
	apply_setup();
	reason = check_current_image(water, iron);
	if (reason.empty() && (image.width != original.width_in_pixels || image.height != original.height_in_pixels))
		reason = "The image has not its first size again";
	if (!reason.empty())
	{
		fail("With the setup of the start again: " + reason);
		return;
	}

	std::cout << "[SUCCESS::XRAY_VIEW::check_image] The X-ray images of the scene are valid, each object attenuates the beam, and the "
				 "changes of the setup take effect"
			  << std::endl;
}
