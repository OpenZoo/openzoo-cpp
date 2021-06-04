#ifndef __N3DS_GPU_H__
#define __N3DS_GPU_H__

#include <3ds.h>
#include <citro3d.h>
#include "../assets.h"

namespace ZZT {
	class N3DSShader {
		DVLB_s *s_dvlb;
		shaderProgram_s s_program;
		int s_geo_count;
		C3D_AttrInfo s_attr;
		int s_reg_id;

	public:
		N3DSShader(const void *data, int size, int geo_count);
		~N3DSShader();

		N3DSShader& addType(GPU_FORMATS format, int count);

		void bind();
		int getGshUniformLocation(const char *name);
	};

	class N3DSCharset {
		C3D_Tex c_texture;
		int g_width, g_height;
		int c_pitch, c_count;
		int tex_width, tex_height;

	public:
		N3DSCharset(Charset &charset);
		~N3DSCharset();

		void bind(int unit = 0);
		uint32_t uv(int glyph) const;

		inline int glyph_width() const { return g_width; }
		inline int glyph_height() const { return g_height; }
		inline int width() const { return tex_width; }
		inline int height() const { return tex_height; }
	};

	struct N3DSTextShaderChar {
		int16_t x, y, z, w;
		uint32_t uv;
		uint32_t col;
	};

	class N3DSTextLayer {
		N3DSShader *l_shader;
		N3DSCharset *l_charset;
		int l_width;
		int l_height;
		N3DSTextShaderChar *l_background;
		N3DSTextShaderChar *l_foreground;
		uint8_t *l_chrs, *l_cols;
		uint32_t l_palette[16];
		C3D_TexEnv c_texenv, c_texenv_inv;
		int l_loc_proj, l_loc_offs, l_loc_texs, l_loc_uvch, l_loc_xych;

	public:
		N3DSTextLayer(N3DSShader *shader, N3DSCharset *charset, int width, int height, const uint32_t *palette, bool transparent);
		~N3DSTextLayer();

		void write(int x, int y, uint8_t chr, uint8_t col, int bg_z = 0, int fg_z = 0);
		void clear(int x, int y);
		void read(int x, int y, uint8_t &chr, uint8_t &col);

		void draw(float x, float y, float z, C3D_Mtx &projection);

		inline int width() const { return l_width; }
		inline int height() const { return l_height; }
		inline int width_px() const { return l_width * l_charset->glyph_width(); }
		inline int height_px() const { return l_height * l_charset->glyph_height(); }
	};
}

#endif