#include "TexProgram.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

Scene::Drawable::Pipeline tex_program_pipeline;

Load<TexProgram> tex_program(LoadTagEarly, []() -> TexProgram const * {
	TexProgram *ret = new TexProgram();

	//----- build the pipeline template -----
    tex_program_pipeline.program = ret->program;

	//make a 1-pixel white texture to bind by default:
	GLuint tex;
	glGenTextures(1, &tex);

	glBindTexture(GL_TEXTURE_2D, tex);
	std::vector<glm::u8vec4> tex_data(1, glm::u8vec4(0xff));
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

    tex_program_pipeline.textures[0].texture = tex;
    tex_program_pipeline.textures[0].target = GL_TEXTURE_2D;

	return ret;
});

// Inspired by Jim McCann's message in the course Discord on how to create a textured quad:
// https://discord.com/channels/1144815260629479486/1154543452520984686/1156347836888256582
TexProgram::TexProgram() {
	//Compile vertex and fragment shaders using the convenient 'gl_compile_program' helper function:
	program = gl_compile_program(
		//vertex shader:
		"#version 330\n"
		"uniform mat4 OBJECT_TO_CLIP;\n"
        "uniform vec4 COLOR;\n"
        "in vec4 Position;\n"
		"in vec2 TexCoord;\n"
		"out vec2 texCoord;\n"
		"void main() {\n"
		"	gl_Position = OBJECT_TO_CLIP * Position;\n"
		"	texCoord = TexCoord;\n"
		"}\n"
	,
		//fragment shader:
		"#version 330\n"
		"uniform sampler2D TEX;\n"
		"uniform vec4 COLOR;\n"
		"in vec2 texCoord;\n"
		"out vec4 fragColor;\n"
		"void main() {\n"
        "   fragColor = texture(TEX, texCoord) * COLOR;\n"
		"}\n"
	);
	//As you can see above, adjacent strings in C/C++ are concatenated.
	//this is very useful for writing long shader programs inline.

	//look up the locations of vertex attributes:
	Position_vec4 = glGetAttribLocation(program, "Position");
	TexCoord_vec2 = glGetAttribLocation(program, "TexCoord");

	//look up the locations of uniforms:
    COLOR_vec4 = glGetUniformLocation(program, "COLOR");
    OBJECT_TO_CLIP_mat4 = glGetUniformLocation(program, "OBJECT_TO_CLIP");
	GLuint TEX_sampler2D = glGetUniformLocation(program, "TEX");

	//set TEX to always refer to texture binding zero:
	glUseProgram(program); //bind program -- glUniform* calls refer to this program now

	glUniform1i(TEX_sampler2D, 0); //set TEX to sample from GL_TEXTURE0

	glUseProgram(0); //unbind program -- glUniform* calls refer to ??? now
}

TexProgram::~TexProgram() {
	glDeleteProgram(program);
	program = 0;
}

