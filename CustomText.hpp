#include <glm/glm.hpp>
#include <hb.h>
#include <hb-ft.h>
#include <map>

struct CustomText{
	static void draw_text(const char* text, glm::vec2 position, float scale, glm::vec3 color);
	struct CharGlyph {
		unsigned int texId;
		float height;
		float width;
	};

	static CharGlyph LoadGlyphTexture(unsigned int c);
	static FT_Face font_face;
	static unsigned int textProgram;
	static unsigned int VAO;
	static unsigned int VBO;
	private:
		static std::map<unsigned int, CharGlyph> glyphMap;
};
