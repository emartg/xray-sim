/*
 * Gvxr_Adapter.cpp
 * This file implements the Gvxr_Adapter class, which is the entry point of the simulator to the gVirtualXRay library.
 * It is the only place that includes the headers of the library (and the OpenGL loader of the library, GLEW).
 */

// GLEW has to be included before any other OpenGL header
#include <GL/glew.h>

#include "xray/Gvxr_Adapter.h"

#include <exception>
#include <map>
#include <memory>
#include <string>
#include <vector>

// PROTOTYPE WORKAROUND: gVirtualXRay only works with a graphics context object that it created itself (it compiles no
// shader and uploads no mesh without one that is "ready"), and it has no way to adopt the OpenGL context that already
// exists, which is the one of the Engine's window. The readiness flag of the context is private, so it is made
// accessible for the include of this one header (the standard headers are included before, so they are not affected).
// The proper fix is a function of the library that adopts the current context: it is requested from the owner of the
// library, and this workaround goes away with it
#define private public
#include "gVirtualXRay/Context.h"
#undef private

#include "gVirtualXRay/Mixture.h"
#include "gVirtualXRay/PolygonMesh.h"
#include "gVirtualXRay/ResourceManager.h"
#include "gVirtualXRay/Units.h"
#include "gVirtualXRay/XRayBeam.h"
#include "gVirtualXRay/XRayDetector.h"
#include "gVirtualXRay/XRayRenderer.h"
#include "gVirtualXRay/gVirtualXRayConfig.h"

namespace
{
	// Number of texture units whose 2D texture binding is saved and restored
	constexpr GLuint SAVED_TEXTURE_UNITS{ 8 };

	// Mixed-precision flag of the renderer: every buffer in full precision (the default of the library's own interface)
	constexpr std::uint8_t FULL_PRECISION_FLAG{ 255 };

	// Saves the state of OpenGL that the library changes when it is created, and restores it when it is destroyed
	class Gl_State_Guard
	{
	public:
		// Constructors
		// ------------
		Gl_State_Guard()
		{
			glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_framebuffer);
			glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_framebuffer);
			glGetIntegerv(GL_VIEWPORT, viewport);
			glGetIntegerv(GL_SCISSOR_BOX, scissor_box);
			glGetIntegerv(GL_CURRENT_PROGRAM, &program);
			glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vertex_array);
			glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &array_buffer);
			glGetIntegerv(GL_ACTIVE_TEXTURE, &active_texture);
			for (GLuint unit = 0; unit < SAVED_TEXTURE_UNITS; ++unit)
			{
				glActiveTexture(GL_TEXTURE0 + unit);
				glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture_2d[unit]);
			}
			glActiveTexture(static_cast<GLenum>(active_texture));

			is_scissor_test_enabled = glIsEnabled(GL_SCISSOR_TEST);
			is_depth_test_enabled   = glIsEnabled(GL_DEPTH_TEST);
			is_blend_enabled        = glIsEnabled(GL_BLEND);
			is_cull_face_enabled    = glIsEnabled(GL_CULL_FACE);
			is_stencil_test_enabled = glIsEnabled(GL_STENCIL_TEST);
			glGetBooleanv(GL_DEPTH_WRITEMASK, &depth_write_mask);
			glGetBooleanv(GL_COLOR_WRITEMASK, color_write_mask);
			glGetIntegerv(GL_DEPTH_FUNC, &depth_func);
			glGetIntegerv(GL_CULL_FACE_MODE, &cull_face_mode);
			glGetIntegerv(GL_FRONT_FACE, &front_face);
			glGetIntegerv(GL_BLEND_SRC_RGB, &blend_source_rgb);
			glGetIntegerv(GL_BLEND_DST_RGB, &blend_destination_rgb);
			glGetIntegerv(GL_BLEND_SRC_ALPHA, &blend_source_alpha);
			glGetIntegerv(GL_BLEND_DST_ALPHA, &blend_destination_alpha);
			glGetIntegerv(GL_BLEND_EQUATION_RGB, &blend_equation_rgb);
			glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &blend_equation_alpha);
			glGetIntegerv(GL_POLYGON_MODE, polygon_mode);
			glGetFloatv(GL_COLOR_CLEAR_VALUE, clear_color);
			glGetIntegerv(GL_PACK_ALIGNMENT, &pack_alignment);
			glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpack_alignment);

			set_default_state();
		}

		Gl_State_Guard(const Gl_State_Guard&)            = delete;
		Gl_State_Guard& operator=(const Gl_State_Guard&) = delete;

		// Destructor
		// ----------
		~Gl_State_Guard()
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(draw_framebuffer));
			glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(read_framebuffer));
			glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
			glScissor(scissor_box[0], scissor_box[1], scissor_box[2], scissor_box[3]);
			glUseProgram(static_cast<GLuint>(program));
			glBindVertexArray(static_cast<GLuint>(vertex_array));
			glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(array_buffer));
			for (GLuint unit = 0; unit < SAVED_TEXTURE_UNITS; ++unit)
			{
				glActiveTexture(GL_TEXTURE0 + unit);
				glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(texture_2d[unit]));
			}
			glActiveTexture(static_cast<GLenum>(active_texture));

			set_capability(GL_SCISSOR_TEST, is_scissor_test_enabled);
			set_capability(GL_DEPTH_TEST, is_depth_test_enabled);
			set_capability(GL_BLEND, is_blend_enabled);
			set_capability(GL_CULL_FACE, is_cull_face_enabled);
			set_capability(GL_STENCIL_TEST, is_stencil_test_enabled);
			glDepthMask(depth_write_mask);
			glColorMask(color_write_mask[0], color_write_mask[1], color_write_mask[2], color_write_mask[3]);
			glDepthFunc(static_cast<GLenum>(depth_func));
			glCullFace(static_cast<GLenum>(cull_face_mode));
			glFrontFace(static_cast<GLenum>(front_face));
			glBlendFuncSeparate(
				static_cast<GLenum>(blend_source_rgb),
				static_cast<GLenum>(blend_destination_rgb),
				static_cast<GLenum>(blend_source_alpha),
				static_cast<GLenum>(blend_destination_alpha));
			glBlendEquationSeparate(static_cast<GLenum>(blend_equation_rgb), static_cast<GLenum>(blend_equation_alpha));
			glPolygonMode(GL_FRONT_AND_BACK, static_cast<GLenum>(polygon_mode[0]));
			glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
			glPixelStorei(GL_PACK_ALIGNMENT, pack_alignment);
			glPixelStorei(GL_UNPACK_ALIGNMENT, unpack_alignment);

			// the library leaves error flags behind, which would be reported by the next check of the application
			while (glGetError() != GL_NO_ERROR)
			{}
		}

	private:
		// Private Static Methods
		// ----------------------
		// Sets the state of a new context: the library was written for a context that nobody has used, and it does not set
		// every setting that it relies on (e.g., the face culling and the depth test of the Engine's scene would make the
		// thickness of an object wrong)
		static void set_default_state()
		{
			glDisable(GL_SCISSOR_TEST);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
			glDisable(GL_CULL_FACE);
			glDisable(GL_STENCIL_TEST);
			glDepthMask(GL_TRUE);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			glDepthFunc(GL_LESS);
			glCullFace(GL_BACK);
			glFrontFace(GL_CCW);
			glBlendFunc(GL_ONE, GL_ZERO);
			glBlendEquation(GL_FUNC_ADD);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glPixelStorei(GL_PACK_ALIGNMENT, 4);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		}

		static void set_capability(GLenum capability, GLboolean is_enabled)
		{
			if (is_enabled)
				glEnable(capability);
			else
				glDisable(capability);
		}

		// Private Attributes
		// ------------------
		GLint     draw_framebuffer{ 0 }, read_framebuffer{ 0 };
		GLint     viewport[4]{}, scissor_box[4]{};
		GLint     program{ 0 }, vertex_array{ 0 }, array_buffer{ 0 }, active_texture{ GL_TEXTURE0 };
		GLint     texture_2d[SAVED_TEXTURE_UNITS]{};
		GLboolean is_scissor_test_enabled{ GL_FALSE }, is_depth_test_enabled{ GL_FALSE }, is_blend_enabled{ GL_FALSE };
		GLboolean is_cull_face_enabled{ GL_FALSE }, is_stencil_test_enabled{ GL_FALSE };
		GLboolean depth_write_mask{ GL_TRUE }, color_write_mask[4]{};
		GLint     depth_func{ GL_LESS }, cull_face_mode{ GL_BACK }, front_face{ GL_CCW };
		GLint     blend_source_rgb{ GL_ONE }, blend_destination_rgb{ GL_ZERO }, blend_source_alpha{ GL_ONE };
		GLint     blend_destination_alpha{ GL_ZERO }, blend_equation_rgb{ GL_FUNC_ADD }, blend_equation_alpha{ GL_FUNC_ADD };
		GLint     polygon_mode[2]{ GL_FILL, GL_FILL };
		GLfloat   clear_color[4]{};
		GLint     pack_alignment{ 4 }, unpack_alignment{ 4 };
	};

	// Returns the description of an exception that was thrown by the library or by the standard library
	std::string describe_current_exception()
	{
		try
		{
			throw;
		}
		catch (const std::exception& exception)
		{
			return exception.what();
		}
		catch (...)
		{
			return "Unknown exception";
		}
	}
} // namespace

// Objects of the library, which need an OpenGL context to be created, used, and destroyed
struct Gvxr_Adapter::Implementation
{
	gVirtualXRay::Context      context; // declared first, so that it is destroyed last (see the workaround above)
	gVirtualXRay::XRayDetector detector;
	gVirtualXRay::XRayBeam     beam;
	gVirtualXRay::XRayRenderer renderer;
	std::vector<std::unique_ptr<gVirtualXRay::PolygonMesh>> meshes;
	unsigned int                                            width{ 0 }, height{ 0 };
	bool                                                    is_initialised{ false };
};

// Constructors
// ------------
Gvxr_Adapter::Gvxr_Adapter() : implementation{ std::make_unique<Implementation>() } {}

// Destructor
// ----------
Gvxr_Adapter::~Gvxr_Adapter()
{
	if (implementation && implementation->is_initialised)
	{
		try
		{
			Gl_State_Guard guard; // the release of the renderer changes the state of OpenGL
			implementation->meshes.clear();
			implementation->renderer.release();
			gVirtualXRay::ResourceManager::getInstance().setGraphicsContext(nullptr);
			implementation->context.m_ready = false; // nothing was created by the context, so there is nothing to destroy
		}
		catch (...)
		{
			// a destructor must not throw: the objects are left to the driver
		}
	}
}

// Public Static Methods
// ---------------------
std::string Gvxr_Adapter::get_version()
{
	return std::string{ gVirtualXRay_VERSION_MAJOR } + "." + gVirtualXRay_VERSION_MINOR + "." + gVirtualXRay_VERSION_PATCH;
}

std::string Gvxr_Adapter::run_self_check()
{
	// hydrogen, helium, lithium, and beryllium, with the weights used by the tests of the library
	const std::map<int, double> weights{ { 1, 1.0 }, { 2, 2.0 }, { 3, 1.0 }, { 4, 2.0 } };
	const gVirtualXRay::Mixture mixture{ weights };

	if (mixture.getLabel() != "H17He33Li17Be33")
	{
		return "Unexpected label of the mixture: " + mixture.getLabel();
	}

	if (!(mixture.getMolarMass() > 0.0))
	{
		return "The molar mass of the mixture is not positive";
	}

	return "";
}

// Public Methods
// --------------
std::string Gvxr_Adapter::initialise(const Xray_Beam& beam, const Xray_Detector& detector)
{
	if (implementation->is_initialised)
	{
		return "The adapter is already initialised";
	}

	try
	{
		// load the OpenGL functions of the library on the current context (the experimental flag is required by the
		// core profile of the Engine)
		glewExperimental        = GL_TRUE;
		const GLenum glew_error = glewInit();
		// GLEW reports that it has no GLX display on contexts that were not created through GLX (e.g., Wayland or
		// EGL), but it has loaded the functions that the context provides
		if (glew_error != GLEW_OK && glew_error != GLEW_ERROR_NO_GLX_DISPLAY)
		{
			return std::string{ "GLEW could not be initialised: " } + reinterpret_cast<const char*>(glewGetErrorString(glew_error));
		}
		while (glGetError() != GL_NO_ERROR) // glewInit leaves an error in a core profile
		{}

		if (detector.width_in_pixels == 0 || detector.height_in_pixels == 0 || !(detector.pixel_size > 0.0))
		{
			return "The detector has no pixels, or a pixel has no size";
		}

		// the numbers of the library are single-precision floats
		const auto to_number = [](double value) {
			return static_cast<gVirtualXRay::RATIONAL_NUMBER>(value * gVirtualXRay::mm);
		};
		const auto to_vector = [&to_number](const std::array<double, 3>& array) {
			return gVirtualXRay::VEC3{ to_number(array[0]), to_number(array[1]), to_number(array[2]) };
		};

		// the point source and the flat detector
		implementation->detector.setXrayPointSource(to_vector(beam.source_position));
		implementation->detector.setDetectorPosition(to_vector(detector.position));
		implementation->detector.setUpVector(to_vector(detector.up_vector).normal());
		implementation->detector.setNumberOfPixels(gVirtualXRay::Vec2ui{ detector.width_in_pixels, detector.height_in_pixels });
		implementation->detector.setResolutionInUnitsOfLengthPerPixel(
			gVirtualXRay::VEC2{ to_number(detector.pixel_size), to_number(detector.pixel_size) });
		implementation->width  = detector.width_in_pixels;
		implementation->height = detector.height_in_pixels;

		// the monochromatic beam
		implementation->beam.clear();
		if (!implementation->beam.addChannelPerPixelAtSDD(beam.photons_per_pixel, beam.energy * gVirtualXRay::keV))
		{
			return "The beam has an invalid energy or number of photons";
		}

		Gl_State_Guard guard; // the initialisation of the renderer creates framebuffers and textures

		// register the current context (the one of the Engine's window) as the graphics context of the library
		implementation->context.m_ready = true;
		gVirtualXRay::ResourceManager::getInstance().setGraphicsContext(&implementation->context);
		implementation->renderer
			.initialise(gVirtualXRay::XRayRenderer::OPENGL, FULL_PRECISION_FLAG, &implementation->detector, &implementation->beam);
		implementation->renderer.enableArtifactFilteringOnGPU();
		implementation->renderer.useNegativeFilteringFlag(false);
		implementation->is_initialised = true; // the renderer is ready once it has meshes (see compute_image)
	}
	catch (...)
	{
		return describe_current_exception();
	}

	return "";
}

std::string Gvxr_Adapter::add_mesh(const Xray_Mesh& mesh)
{
	if (!implementation->is_initialised)
	{
		return "The adapter is not initialised";
	}
	if (mesh.positions.empty() || mesh.positions.size() % 3 != 0 || mesh.indices.empty() || mesh.indices.size() % 3 != 0)
	{
		return "The mesh '" + mesh.name + "' needs positions and indices in groups of three";
	}
	for (const std::uint32_t index : mesh.indices)
	{
		if (index >= mesh.positions.size() / 3)
		{
			return "The mesh '" + mesh.name + "' has an index outside of its vertices";
		}
	}

	try
	{
		Gl_State_Guard guard; // the upload of the mesh changes the buffer bindings

		auto polygon_mesh = std::make_unique<gVirtualXRay::PolygonMesh>();
		polygon_mesh->setInternalData(
			GL_TRIANGLES,
			mesh.positions.data(),
			static_cast<unsigned int>(mesh.positions.size() / 3),
			GL_FLOAT,
			mesh.indices.data(),
			static_cast<unsigned int>(mesh.indices.size()),
			GL_UNSIGNED_INT,
			true,
			GL_STATIC_DRAW);
		polygon_mesh->setCompound(mesh.material.compound);
		polygon_mesh->setDensity(mesh.material.density * gVirtualXRay::g / gVirtualXRay::cm3);

		implementation->renderer.addInnerSurface(polygon_mesh.get());
		implementation->renderer.needUpdateSurfacePerMaterial();
		implementation->meshes.push_back(std::move(polygon_mesh));
	}
	catch (...)
	{
		return describe_current_exception();
	}

	return "";
}

std::string Gvxr_Adapter::compute_image(Xray_Image& image)
{
	if (!implementation->is_initialised)
	{
		return "The adapter is not initialised";
	}

	try
	{
		Gl_State_Guard guard; // the computation binds its own framebuffers, programs, and textures

		if (!implementation->renderer.isReady())
		{
			return "The X-ray renderer is not ready (it needs a valid detector, beam, and shaders, and at least one mesh)";
		}

		implementation->renderer.computeImage(gVirtualXRay::MATRIX4{}, true);

		image.width  = implementation->width;
		image.height = implementation->height;
		image.pixels.assign(static_cast<std::size_t>(image.width) * image.height, 0.0f);
		implementation->renderer.getFBO(XRAY_DETECTOR_ENERGY_FLUENCE_FBO_ID, image.pixels.data(), image.width, image.height);
	}
	catch (...)
	{
		return describe_current_exception();
	}

	return "";
}

// Getters
// -------
bool Gvxr_Adapter::get_is_initialised() const
{
	return implementation->is_initialised;
}
