#include "CustomText.hpp"
#include "GL.hpp"
#include "Load.hpp"
#include "freetype/freetype.h"
#include "gl_compile_program.hpp"
#include "gl_errors.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>

FT_Face CustomText::font_face;
unsigned int CustomText::textProgram;
unsigned int CustomText::VAO;
unsigned int CustomText::VBO;
std::map<unsigned int, CustomText::CharGlyph> CustomText::glyphMap;

// External code sources: Mostly cobbled together from
// https://learnopengl.com/In-Practice/Text-Rendering
// and https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c

static Load< void > setup_fontface(LoadTagDefault, [](){
	const char *fontfile;
	fontfile = "font.otf";

	/* Initialize FreeType and create FreeType font face. */
	FT_Library ft_library;
	FT_Face ft_face;
	FT_Error ft_error;
	ft_error = FT_Init_FreeType (&ft_library);
	if (ft_error)
		abort();
	ft_error = FT_New_Face (ft_library, fontfile, 0, &ft_face);
	if (ft_error)
		abort();
	ft_error = FT_Set_Pixel_Sizes (ft_face, 0, 24);
	if (ft_error)
		abort();

	CustomText::textProgram = gl_compile_program(
			"#version 330 core\n"
			"layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>\n"
			"out vec2 TexCoords;\n"

			"uniform mat4 projection;\n"

			"void main()\n"
			"{\n"
				"gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
				"TexCoords = vertex.zw;\n"
			"}\n",
			"#version 330 core\n"
			"in vec2 TexCoords;\n"
			"out vec4 color;\n"

			"uniform sampler2D text;\n"
			"uniform vec3 textColor;\n"

			"void main()\n"
			"{\n"
				"vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\n"
				"color = vec4(1.0) * sampled;\n"
			"}\n"
			);

	glUseProgram(CustomText::textProgram);
	glGenVertexArrays(1, &CustomText::VAO);
	glGenBuffers(1, &CustomText::VBO);
	glBindVertexArray(CustomText::VAO);
	glBindBuffer(GL_ARRAY_BUFFER, CustomText::VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CustomText::font_face = ft_face;
	GL_ERRORS(); //PARANOIA: make sure nothing strange happened during setup
});

CustomText::CharGlyph CustomText::LoadGlyphTexture(unsigned int c){
	auto found = glyphMap.find(c);
	if(found != glyphMap.end())
		return found->second;

	// load character glyph 
    if (FT_Load_Glyph(font_face, c, FT_LOAD_RENDER))
    {
        std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
        return CustomText::CharGlyph();
    }
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // generate texture
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        font_face->glyph->bitmap.width,
        font_face->glyph->bitmap.rows,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        font_face->glyph->bitmap.buffer
    );
    // set texture options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // now store character for later use
	CharGlyph glyph;
	glyph.texId = texture;
	glyph.height = (float)font_face->glyph->bitmap.rows;
	glyph.width = (float)font_face->glyph->bitmap.width;
    glyphMap.insert(std::pair<unsigned int, CharGlyph>(c, glyph));
	
	return glyph;
}

void CustomText::draw_text(const char* intext, glm::vec2 position, float scale, glm::vec3 color){	   
	glUseProgram(textProgram);
	glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	std::string fullText = intext;
	std::string remaining("");
	std::string nextText = fullText;
	auto nextLineBreak = fullText.find('\n');
	if(nextLineBreak != std::string::npos){	
		remaining = fullText.substr(nextLineBreak + 1);
		nextText = fullText.substr(0, nextLineBreak);
	}
	GLuint matrix = glGetUniformLocation(CustomText::textProgram, "projection");
	glm::mat4 ourProj = glm::ortho(0.0f, 1280.0f, 0.0f, 720.0f, -1.0f, 1.0f);
	glUniformMatrix4fv(matrix, 1, GL_FALSE, glm::value_ptr(ourProj));
	GLuint textColor = glGetUniformLocation(CustomText::textProgram, "textColor");
	glUniform3fv(textColor, 1, glm::value_ptr(color));

	/* Create hb-ft font. */
	hb_font_t *hb_font;
	hb_font = hb_ft_font_create (font_face, NULL);

	/* Create hb-buffer and populate. */
	hb_buffer_t *hb_buffer;
	hb_buffer = hb_buffer_create ();
	hb_buffer_add_utf8 (hb_buffer, nextText.c_str(), -1, 0, -1);
	hb_buffer_guess_segment_properties (hb_buffer);

	/* Shape it! */
	hb_shape (hb_font, hb_buffer, NULL, 0);

	/* Get glyph information and positions out of the buffer. */
	unsigned int len = hb_buffer_get_length (hb_buffer);
	hb_glyph_info_t *info = hb_buffer_get_glyph_infos (hb_buffer, NULL);
	hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (hb_buffer, NULL);


	float x = position.x;
	float y = position.y;

	for(unsigned int i = 0; i < len; i++){
		CharGlyph glyph = LoadGlyphTexture(info[i].codepoint);

		float xpos = x + (float)(pos[i].x_offset / 64.);
		float ypos = y + (float)(pos[i].y_offset / 64.);

		float h = glyph.height;
		float w = glyph.width;

        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },            
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }           
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, glyph.texId);
        // update content of VBO memory
		glUniform1i(glGetUniformLocation(textProgram, "text"), 0);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		x += (float)(pos[i].x_advance / 64.);
		y += (float)(pos[i].y_advance / 64.);
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	if(remaining.length() > 0){	
		draw_text(remaining.c_str(), position + glm::vec2(0, -50), scale, color);
	}
}
