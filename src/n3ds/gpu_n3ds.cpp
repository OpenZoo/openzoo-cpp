#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "gpu_n3ds.h"
#include "../utils/mathutils.h"

using namespace ZZT;

N3DSShader::N3DSShader(const void *data, int size, int geo_count) {
	this->s_dvlb = DVLB_ParseFile((uint32_t*) data, size);
	this->s_geo_count = geo_count;
	this->s_reg_id = 0;
	shaderProgramInit(&this->s_program);
	shaderProgramSetVsh(&this->s_program, &this->s_dvlb->DVLE[0]);
	shaderProgramSetGsh(&this->s_program, &this->s_dvlb->DVLE[1], geo_count);
	AttrInfo_Init(&this->s_attr);
}

N3DSShader::~N3DSShader() {
	// TODO
}

N3DSShader& N3DSShader::addType(GPU_FORMATS format, int count) {
	AttrInfo_AddLoader(&this->s_attr, s_reg_id++, format, count);
	return *this;
}

void N3DSShader::bind() {
	C3D_BindProgram(&this->s_program);
	C3D_SetAttrInfo(&this->s_attr);
}

int N3DSShader::getGshUniformLocation(const char *name) {
	return shaderInstanceGetUniformLocation(this->s_program.geometryShader, name);
}

//

N3DSCharset::N3DSCharset(Charset &charset) {
	this->g_width = charset.width();
	this->g_height = charset.height();
	this->c_pitch = 32;
	this->c_count = 256;

	this->tex_width = (this->c_pitch) * this->g_width;
	this->tex_height = ((this->c_count + this->c_pitch - 1) / this->c_pitch) * this->g_height;

	tex_width = NextPowerOfTwo(tex_width);
	tex_height = NextPowerOfTwo(tex_height);

	C3D_TexInitVRAM(&this->c_texture, tex_width, tex_height, GPU_RGBA8);
	C3D_TexSetFilter(&this->c_texture, GPU_NEAREST, GPU_NEAREST);

	uint32_t *buf = (uint32_t*) linearAlloc(tex_width * tex_height * sizeof(uint32_t));

	Charset::Iterator charsetIterator = charset.iterate();
	while (charsetIterator.next()) {
		uint32_t color = charsetIterator.value | 0xFFFFFF00;
		int cx = (charsetIterator.glyph % this->c_pitch) * this->g_width + charsetIterator.x;
		int cy = (charsetIterator.glyph / this->c_pitch) * this->g_height + charsetIterator.y;
		buf[cy * tex_width + cx] = color;
	}

	GSPGPU_FlushDataCache(buf, tex_width * tex_height * sizeof(uint32_t));
	C3D_SyncDisplayTransfer(
		buf,
		GX_BUFFER_DIM(tex_width, tex_height),
		(uint32_t*) this->c_texture.data,
		GX_BUFFER_DIM(tex_width, tex_height),
		(
			GX_TRANSFER_FLIP_VERT(0)
			| GX_TRANSFER_OUT_TILED(1)
			| GX_TRANSFER_RAW_COPY(0)
			| GX_TRANSFER_IN_FORMAT(GPU_RGBA8)
			| GX_TRANSFER_OUT_FORMAT(GPU_RGBA8)
			| GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO)
		)
	);

	linearFree(buf);
}

N3DSCharset::~N3DSCharset() {
	C3D_TexDelete(&this->c_texture);
}

void N3DSCharset::bind(int unit) {
	C3D_TexBind(unit, &this->c_texture);
}

uint32_t N3DSCharset::uv(int glyph) const {
	uint16_t u = (glyph % this->c_pitch) * this->g_width;
	uint16_t v = this->tex_height - ((glyph / this->c_pitch) * this->g_height);
	return (v << 16) | u;
}

//

N3DSTextLayer::N3DSTextLayer(N3DSShader *shader, N3DSCharset *charset, int width, int height, const uint32_t *palette, bool transparent) {
	this->l_shader = shader;
	this->l_charset = charset;
	this->l_width = width;
	this->l_height = height;
	this->l_background = (N3DSTextShaderChar*) linearAlloc(sizeof(N3DSTextShaderChar) * l_width * l_height * 2);
	this->l_foreground = this->l_background + (l_width * l_height);
	this->l_chrs = (uint8_t*) malloc(l_width * l_height);
	this->l_cols = (uint8_t*) malloc(l_width * l_height);

	memset(this->l_chrs, 0, l_width * l_height);
	memset(this->l_cols, 0, l_width * l_height);

	for (int i = 0; i < 16; i++) {
		this->l_palette[i] = (palette[i] << 8) | 0xFF;
	}

	int i = 0;
	uint32_t fg_uv = charset->uv(0);
	for (int iy = 0; iy < height; iy++) {
		for (int ix = 0; ix < width; ix++, i++) {
			this->l_background[i].x = ix;
			this->l_background[i].y = iy;
			this->l_background[i].z = 0;
			
			this->l_background[i].uv = fg_uv;
			this->l_background[i].col = 0;

			this->l_foreground[i].x = ix;
			this->l_foreground[i].y = iy;
			this->l_foreground[i].z = 0;

			this->l_foreground[i].uv = fg_uv;
			this->l_foreground[i].col = 0;
		}
	}

	C3D_TexEnvInit(&(this->c_texenv));
	C3D_TexEnvSrc(&(this->c_texenv), C3D_RGB, (GPU_TEVSRC) 0, GPU_PRIMARY_COLOR, (GPU_TEVSRC) 0);
	C3D_TexEnvSrc(&(this->c_texenv), C3D_Alpha, (GPU_TEVSRC) 0, GPU_TEXTURE0, (GPU_TEVSRC) 0);
	C3D_TexEnvOpRgb(&(this->c_texenv), (GPU_TEVOP_RGB) 0, GPU_TEVOP_RGB_SRC_ALPHA, (GPU_TEVOP_RGB) 0);
	C3D_TexEnvOpAlpha(&(this->c_texenv), (GPU_TEVOP_A) 0, GPU_TEVOP_A_SRC_ALPHA, (GPU_TEVOP_A) 0);
	C3D_TexEnvFunc(&(this->c_texenv), C3D_Both, GPU_MODULATE);

	C3D_TexEnvInit(&(this->c_texenv_inv));
	C3D_TexEnvSrc(&(this->c_texenv_inv), C3D_RGB, (GPU_TEVSRC) 0, GPU_PRIMARY_COLOR, (GPU_TEVSRC) 0);
	C3D_TexEnvSrc(&(this->c_texenv_inv), C3D_Alpha, (GPU_TEVSRC) 0, GPU_TEXTURE0, (GPU_TEVSRC) 0);
	C3D_TexEnvOpRgb(&(this->c_texenv_inv), (GPU_TEVOP_RGB) 0, GPU_TEVOP_RGB_SRC_ALPHA, (GPU_TEVOP_RGB) 0);
	C3D_TexEnvOpAlpha(&(this->c_texenv_inv), (GPU_TEVOP_A) 0, GPU_TEVOP_A_ONE_MINUS_SRC_ALPHA, (GPU_TEVOP_A) 0);
	C3D_TexEnvFunc(&(this->c_texenv_inv), C3D_Both, GPU_MODULATE);

	l_loc_proj = l_shader->getGshUniformLocation("projection");
	l_loc_offs = l_shader->getGshUniformLocation("offset");
	l_loc_texs = l_shader->getGshUniformLocation("tex_scale");
	l_loc_uvch = l_shader->getGshUniformLocation("uv_char_offset");
	l_loc_xych = l_shader->getGshUniformLocation("xy_char_offset");
}

void N3DSTextLayer::write(int x, int y, uint8_t chr, uint8_t col, int bg_z, int fg_z) {
	if (x < 0 || y < 0 || x >= l_width || y >= l_height) {
		return;
	}

	int i = y * l_width + x;
	l_background[i].z = bg_z;
	l_background[i].uv = l_charset->uv(chr);
	l_background[i].col = l_palette[(col >> 4) & 0x7];
	l_foreground[i].z = fg_z;
	l_foreground[i].uv = l_charset->uv(chr);
	l_foreground[i].col = l_palette[col & 0xF];
	l_chrs[i] = chr;
	l_cols[i] = col;
}

void N3DSTextLayer::clear(int x, int y) {
	if (x < 0 || y < 0 || x >= l_width || y >= l_height) {
		return;
	}

	int i = y * l_width + x;
	l_background[i].col = 0;
	l_foreground[i].col = 0;
	l_chrs[i] = 0;
	l_cols[i] = 0;
}

void N3DSTextLayer::read(int x, int y, uint8_t &chr, uint8_t &col) {
	if (x < 0 || y < 0 || x >= l_width || y >= l_height) {
		chr = 0;
		col = 0;
	}

	int i = y * l_width + x;
	chr = l_chrs[i];
	col = l_cols[i];
}

void N3DSTextLayer::draw(float x, float y, float z, C3D_Mtx &projection) {
	C3D_BufInfo *bufInfo = C3D_GetBufInfo();

	l_shader->bind();
	C3D_FVUnifMtx4x4(GPU_GEOMETRY_SHADER, l_loc_proj, &projection);
	C3D_FVUnifSet(GPU_GEOMETRY_SHADER, l_loc_offs, x, y, z, 0.0f);
	C3D_FVUnifSet(GPU_GEOMETRY_SHADER, l_loc_texs, 1.0f / l_charset->width(), 1.0f / l_charset->height(), -1.0f, 1.0f);
	C3D_FVUnifSet(GPU_GEOMETRY_SHADER, l_loc_uvch, l_charset->glyph_width(), -l_charset->glyph_height(), 0.0f, 0.0f);
	C3D_FVUnifSet(GPU_GEOMETRY_SHADER, l_loc_xych, l_charset->glyph_width(), l_charset->glyph_height(), 1.0f, 1.0f);

	GSPGPU_FlushDataCache(this->l_background, l_width * l_height * sizeof(N3DSTextShaderChar));
	GSPGPU_FlushDataCache(this->l_foreground, l_width * l_height * sizeof(N3DSTextShaderChar));

	l_charset->bind(0);

	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, this->l_background, sizeof(N3DSTextShaderChar), 3, 0x210);
	
	C3D_SetTexEnv(0, &(this->c_texenv_inv));
	C3D_DrawArrays(GPU_GEOMETRY_PRIM, 0, l_width * l_height);

	C3D_SetTexEnv(0, &(this->c_texenv));
	C3D_DrawArrays(GPU_GEOMETRY_PRIM, l_width * l_height, l_width * l_height);
}

N3DSTextLayer::~N3DSTextLayer() {
	linearFree(this->l_background);
	free(this->l_chrs);
	free(this->l_cols);
}