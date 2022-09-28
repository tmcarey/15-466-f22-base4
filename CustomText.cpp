#include "CustomText.hpp"
#include "Load.hpp"
#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

FT_Face CustomText::font_face;
unsigned int CustomText::textProgram;
unsigned int CustomText::VAO;
unsigned int CustomText::VBO;
std::map<unsigned int, CustomText::CharGlyph> CustomText::glyphMap;

static Load< void > setup_fontface(LoadTagDefault, [](){
	const char *fontfile;
	fontfile = "font.otf";

	/* Initialize FreeType and create FreeType font face. */
	FT_Library ft_library;
	FT_Face ft_face;
	FT_Error ft_error;
	int FONT_SIZE = 12;
	ft_error = FT_Init_FreeType (&ft_library);
	if (ft_error)
		abort();
	ft_error = FT_New_Face (ft_library, fontfile, 0, &ft_face);
	if (ft_error)
		abort();
	ft_error = FT_Set_Char_Size (ft_face, FONT_SIZE*64, FONT_SIZE*64, 0, 0);
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
				"color = vec4(textColor, 1.0) * sampled;\n"
			"}\n"
			);
	printf("Compiled Shadres\n");

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
	printf("Loading glyph %d\n", c);

	// load character glyph 
    if (FT_Load_Char(font_face, c, FT_LOAD_RENDER))
    {
        std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
        return CustomText::CharGlyph();
    }
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
	glyph.height = (float)font_face->glyph->bitmap.width;
	glyph.width = (float)font_face->glyph->bitmap.rows;
    glyphMap.insert(std::pair<unsigned int, CharGlyph>(c, glyph));
	
	return glyph;
}

void CustomText::draw_text(float x, float y, float scale, glm::vec3 color){	
	const char* text = "Hello World";
	glUseProgram(textProgram);
	glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);
	

	/* Create hb-ft font. */
	hb_font_t *hb_font;
	hb_font = hb_ft_font_create (font_face, NULL);

	/* Create hb-buffer and populate. */
	hb_buffer_t *hb_buffer;
	hb_buffer = hb_buffer_create ();
	hb_buffer_add_utf8 (hb_buffer, text, -1, 0, -1);
	hb_buffer_guess_segment_properties (hb_buffer);

	/* Shape it! */
	hb_shape (hb_font, hb_buffer, NULL, 0);

	/* Get glyph information and positions out of the buffer. */
	unsigned int len = hb_buffer_get_length (hb_buffer);
	hb_glyph_info_t *info = hb_buffer_get_glyph_infos (hb_buffer, NULL);
	hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (hb_buffer, NULL);

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
}
