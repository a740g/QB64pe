//-----------------------------------------------------------------------------------------------------
// FreeType PSF Driver for QB64-PE by a740g
//-----------------------------------------------------------------------------------------------------

#include "sg_psf.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MODULE_H
#include <freetype/internal/ftdebug.h>
#include <freetype/internal/ftdrv.h>
#include <freetype/internal/ftmemory.h>
#include <freetype/internal/ftobjs.h>
#include <freetype/internal/ftstream.h>

extern FT_Short codepage437_to_unicode16[];

#undef FT_COMPONENT
#define FT_COMPONENT psf

#define PSF1_MAGIC 0x0436
#define PSF1_MODE512 0x01
#define PSF1_MODEHASTAB 0x02
#define PSF1_MODEHASSEQ 0x04

#define PSF2_MAGIC 0x864ab572
#define PSF2_HAS_UNICODE_TABLE 0x01

#define PSF_LE32(b) ((FT_ULong)(b)[0] | ((FT_ULong)(b)[1] << 8) | ((FT_ULong)(b)[2] << 16) | ((FT_ULong)(b)[3] << 24))

typedef struct PSF_FaceRec_ {
    FT_FaceRec root;
    FT_UInt glyph_width;
    FT_UInt glyph_height;
    FT_UInt bytes_per_glyph;
    FT_Long num_file_glyphs;
    FT_UInt ascent;
    FT_Byte *glyph_data;
    FT_UShort unicode_map[65536];
} PSF_FaceRec, *PSF_Face;

typedef struct PSF_CMapRec_ {
    FT_CMapRec root;
} PSF_CMapRec, *PSF_CMap;

FT_CALLBACK_DEF(FT_Error) psf_cmap_init(FT_CMap cmap, FT_Pointer init_data) {
    FT_UNUSED(cmap);
    FT_UNUSED(init_data);
    return FT_Err_Ok;
}

FT_CALLBACK_DEF(void) psf_cmap_done(FT_CMap cmap) {
    FT_UNUSED(cmap);
}

FT_CALLBACK_DEF(FT_UInt) psf_cmap_char_index(FT_CMap cmap, FT_UInt32 char_code) {
    PSF_Face face = (PSF_Face)FT_CMAP_FACE(cmap);
    if (char_code < 65536) {
        return face->unicode_map[char_code];
    }
    return 0;
}

FT_CALLBACK_DEF(FT_UInt) psf_cmap_char_next(FT_CMap cmap, FT_UInt32 *achar_code) {
    PSF_Face face = (PSF_Face)FT_CMAP_FACE(cmap);
    FT_UInt32 code;

    if (*achar_code >= 0xFFFFUL) {
        *achar_code = 0;
        return 0;
    }

    code = *achar_code + 1;
    while (code < 65536) {
        if (face->unicode_map[code] != 0) {
            *achar_code = code;
            return face->unicode_map[code];
        }
        code++;
    }
    *achar_code = 0;
    return 0;
}

static const FT_CMap_ClassRec psf_cmap_class = {
    sizeof(PSF_CMapRec), psf_cmap_init, psf_cmap_done, psf_cmap_char_index, psf_cmap_char_next, NULL, NULL, NULL, NULL, NULL};

FT_CALLBACK_DEF(void) psf_face_done(FT_Face ft_face) {
    PSF_Face face = (PSF_Face)ft_face;
    FT_Memory memory;

    if (!face)
        return;

    memory = FT_FACE_MEMORY(face);
    FT_FREE(face->glyph_data);
    FT_FREE(ft_face->available_sizes);
    FT_FREE(ft_face->family_name);
    FT_FREE(ft_face->style_name);
}

FT_CALLBACK_DEF(FT_Error) psf_face_init(FT_Stream stream, FT_Face ft_face, FT_Int face_index, FT_Int num_params, FT_Parameter *params) {
    PSF_Face face = (PSF_Face)ft_face;
    FT_Memory memory = FT_FACE_MEMORY(face);
    FT_Error error = FT_Err_Ok;
    FT_ULong magic;
    FT_UInt version = 0, has_table = 0;
    FT_ULong header_size = 0, data_size = 0;
    FT_Byte buf[28];
    FT_ULong row_bytes;
    FT_Long i;

    FT_UNUSED(num_params);
    FT_UNUSED(params);

    if (FT_STREAM_SEEK(0) || FT_READ_ULONG_LE(magic))
        return FT_THROW(Unknown_File_Format);

    if ((magic & 0xFFFF) == PSF1_MAGIC) {
        FT_Byte mode = (magic >> 16) & 0xFF;
        FT_Byte charsize = (magic >> 24) & 0xFF;

        if (mode > 0x07)
            return FT_THROW(Unknown_File_Format);

        version = 1;
        header_size = 4;
        face->glyph_width = 8;
        face->glyph_height = charsize;
        face->num_file_glyphs = (mode & PSF1_MODE512) ? 512 : 256;
        face->bytes_per_glyph = charsize;
        has_table = (mode & (PSF1_MODEHASTAB | PSF1_MODEHASSEQ)) != 0;
    } else if (magic == PSF2_MAGIC) {
        FT_ULong n;
        if (FT_STREAM_READ(buf, 28)) {
            return FT_THROW(Unknown_File_Format);
        }

        if (PSF_LE32(buf) != 0) {
            return FT_THROW(Unknown_File_Format);
        }

        version = 2;
        header_size = PSF_LE32(buf + 4);
        has_table = (buf[8] & PSF2_HAS_UNICODE_TABLE) != 0;

        n = PSF_LE32(buf + 12);
        if (n == 0 || n > 65534)
            return FT_THROW(Invalid_File_Format);

        face->num_file_glyphs = (FT_Long)n;
        face->bytes_per_glyph = (FT_UInt)PSF_LE32(buf + 16);
        face->glyph_height = (FT_UInt)PSF_LE32(buf + 20);
        face->glyph_width = (FT_UInt)PSF_LE32(buf + 24);
    } else {
        return FT_THROW(Unknown_File_Format);
    }

    ft_face->num_glyphs = face->num_file_glyphs + 1;

    row_bytes = (face->glyph_width + 7) / 8;
    if (face->glyph_width == 0 || face->glyph_height == 0 || face->glyph_width > 0x7FFF || face->glyph_height > 0x7FFF || face->num_file_glyphs <= 0 ||
        face->num_file_glyphs > 65534 || face->bytes_per_glyph < face->glyph_height * row_bytes) {
        return version == 1 ? FT_THROW(Unknown_File_Format) : FT_THROW(Invalid_File_Format);
    }

    face->ascent = face->glyph_height * 4 / 5;

    ft_face->height = (FT_Short)face->glyph_height;
    ft_face->ascender = (FT_Short)face->ascent;
    ft_face->descender = (FT_Short)face->ascent - (FT_Short)face->glyph_height;
    ft_face->max_advance_width = (FT_Short)face->glyph_width;

    if (version == 2 && header_size < 32)
        return FT_THROW(Invalid_File_Format);

    if (header_size > stream->size || (FT_ULong)face->num_file_glyphs > (stream->size - header_size) / face->bytes_per_glyph) {
        return version == 1 ? FT_THROW(Unknown_File_Format) : FT_THROW(Invalid_File_Format);
    }

    ft_face->num_faces = 1;
    if (face_index < 0)
        return FT_Err_Ok;

    if ((face_index & 0xFFFF) > 0)
        return FT_THROW(Invalid_Argument);

    ft_face->face_index = 0;
    ft_face->face_flags = FT_FACE_FLAG_FIXED_SIZES | FT_FACE_FLAG_FIXED_WIDTH | FT_FACE_FLAG_HORIZONTAL;

    {
        char family_buf[32];
        ft_snprintf(family_buf, sizeof(family_buf), "PSF %ux%u", face->glyph_width, face->glyph_height);

        if (FT_STRDUP(ft_face->family_name, family_buf) || FT_STRDUP(ft_face->style_name, "Regular")) {
            goto Fail;
        }
    }

    if (FT_NEW_ARRAY(ft_face->available_sizes, 1)) {
        goto Fail;
    }

    ft_face->num_fixed_sizes = 1;
    ft_face->available_sizes[0].width = (FT_Short)face->glyph_width;
    ft_face->available_sizes[0].height = (FT_Short)face->glyph_height;
    ft_face->available_sizes[0].size = (FT_Pos)(face->glyph_height << 6);
    ft_face->available_sizes[0].x_ppem = (FT_Pos)(face->glyph_height << 6);
    ft_face->available_sizes[0].y_ppem = (FT_Pos)(face->glyph_height << 6);

    data_size = (FT_ULong)face->num_file_glyphs * face->bytes_per_glyph;

    if (FT_STREAM_SEEK(header_size) || FT_QALLOC(face->glyph_data, data_size)) {
        goto Fail;
    }

    if (FT_STREAM_READ(face->glyph_data, data_size)) {
        goto Fail;
    }

    FT_MEM_ZERO(face->unicode_map, sizeof(face->unicode_map));

    if (!has_table) {
        for (i = 0; i < face->num_file_glyphs && i < 256; i++) {
            FT_UInt32 cp = codepage437_to_unicode16[i];
            if (cp != 0 && face->unicode_map[cp] == 0) {
                face->unicode_map[cp] = (FT_UShort)(i + 1);
            }
        }
    } else {
        FT_ULong table_offset = header_size + data_size;
        FT_ULong table_size;
        FT_Byte *table = NULL;

        if (table_offset < stream->size) {
            table_size = stream->size - table_offset;

            if (FT_STREAM_SEEK(table_offset) || FT_QALLOC(table, table_size) || FT_STREAM_READ(table, table_size)) {
                FT_FREE(table);
                goto Fail;
            }

            {
                FT_Long g;
                if (version == 1) {
                    const FT_Byte *p = table;
                    const FT_Byte *end = table + (table_size & ~(FT_ULong)1);

                    for (g = 0; g < face->num_file_glyphs && p < end; g++) {
                        int in_seq = 0;
                        while (p < end) {
                            FT_UShort val = (FT_UShort)(p[0] | (p[1] << 8));
                            p += 2;
                            if (val == 0xFFFF)
                                break;
                            if (val == 0xFFFE) {
                                in_seq = 1;
                                continue;
                            }
                            if (!in_seq && val != 0 && face->unicode_map[val] == 0)
                                face->unicode_map[val] = (FT_UShort)(g + 1);
                        }
                    }
                } else {
                    const FT_Byte *p = table;
                    const FT_Byte *end = table + table_size;

                    for (g = 0; g < face->num_file_glyphs && p < end; g++) {
                        int in_seq = 0;
                        while (p < end) {
                            FT_UInt32 cp = 0;
                            FT_Byte c;
                            int ok = 0;
                            if (*p == 0xFF) {
                                p++;
                                break;
                            }
                            if (*p == 0xFE) {
                                p++;
                                in_seq = 1;
                                continue;
                            }
                            c = *p++;
                            if (c < 0x80) {
                                cp = c;
                                ok = 1;
                            } else if ((c & 0xE0) == 0xC0 && p < end && (p[0] & 0xC0) == 0x80) {
                                cp = ((FT_UInt32)(c & 0x1F) << 6) | (p[0] & 0x3F);
                                p += 1;
                                ok = cp >= 0x80;
                            } else if ((c & 0xF0) == 0xE0 && p + 1 < end && (p[0] & 0xC0) == 0x80 && (p[1] & 0xC0) == 0x80) {
                                cp = ((FT_UInt32)(c & 0x0F) << 12) | ((p[0] & 0x3F) << 6) | (p[1] & 0x3F);
                                p += 2;
                                ok = cp >= 0x800 && (cp < 0xD800 || cp > 0xDFFF);
                            } else if ((c & 0xF8) == 0xF0 && p + 2 < end && (p[0] & 0xC0) == 0x80 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
                                p += 3;
                            }

                            if (ok && !in_seq && cp != 0 && cp < 65536 && face->unicode_map[cp] == 0)
                                face->unicode_map[cp] = (FT_UShort)(g + 1);
                        }
                    }
                }
            }
            FT_FREE(table);
        }
    }

    {
        FT_CharMapRec charmap;
        charmap.face = ft_face;
        charmap.encoding = FT_ENCODING_UNICODE;
        charmap.platform_id = 3;
        charmap.encoding_id = 1;
        error = FT_CMap_New(&psf_cmap_class, NULL, &charmap, NULL);
        if (error)
            goto Fail;
    }

    return FT_Err_Ok;

Fail:
    return error ? error : FT_THROW(Unknown_File_Format);
}

FT_CALLBACK_DEF(FT_Error) psf_size_select(FT_Size size, FT_ULong strike_index) {
    PSF_Face face = (PSF_Face)size->face;
    FT_UInt asc = face->ascent;

    FT_Select_Metrics(size->face, strike_index);
    size->metrics.ascender = (FT_Pos)(asc << 6);
    size->metrics.descender = -(FT_Pos)((face->glyph_height - asc) << 6);
    size->metrics.max_advance = (FT_Pos)(face->glyph_width << 6);
    return FT_Err_Ok;
}

FT_CALLBACK_DEF(FT_Error) psf_size_request(FT_Size size, FT_Size_Request req) {
    FT_ULong idx;
    FT_Error error = FT_Match_Size(size->face, req, 1, &idx);
    if (error)
        return error;
    return psf_size_select(size, idx);
}

FT_CALLBACK_DEF(FT_Error) psf_glyph_load(FT_GlyphSlot slot, FT_Size size, FT_UInt glyph_index, FT_Int32 load_flags) {
    PSF_Face face = (PSF_Face)slot->face;
    FT_Bitmap *bitmap = &slot->bitmap;
    FT_UInt bytes_per_row;
    const FT_Byte *src;
    FT_Error error;

    FT_UNUSED(size);
    FT_UNUSED(load_flags);

    if (glyph_index >= (FT_UInt)slot->face->num_glyphs) {
        return FT_THROW(Invalid_Argument);
    }

    bytes_per_row = (face->glyph_width + 7) / 8;

    bitmap->rows = face->glyph_height;
    bitmap->width = face->glyph_width;
    bitmap->pitch = (int)bytes_per_row;
    bitmap->pixel_mode = FT_PIXEL_MODE_MONO;
    bitmap->num_grays = 2;

    error = ft_glyphslot_alloc_bitmap(slot);
    if (error)
        return error;

    if (glyph_index == 0) {
        FT_MEM_ZERO(bitmap->buffer, (size_t)bitmap->rows * bytes_per_row);
    } else {
        src = face->glyph_data + ((size_t)(glyph_index - 1) * face->bytes_per_glyph);
        FT_MEM_COPY(bitmap->buffer, src, (size_t)bitmap->rows * bytes_per_row);
    }

    slot->format = FT_GLYPH_FORMAT_BITMAP;
    slot->bitmap_left = 0;
    slot->bitmap_top = (FT_Int)face->ascent;

    slot->metrics.width = (FT_Pos)(face->glyph_width << 6);
    slot->metrics.height = (FT_Pos)(face->glyph_height << 6);
    slot->metrics.horiBearingX = 0;
    slot->metrics.horiBearingY = (FT_Pos)(face->ascent << 6);
    slot->metrics.horiAdvance = (FT_Pos)(face->glyph_width << 6);
    slot->linearHoriAdvance = (FT_Fixed)(face->glyph_width << 16);
    slot->linearVertAdvance = (FT_Fixed)(face->glyph_height << 16);

    ft_synthesize_vertical_metrics(&slot->metrics, (FT_Pos)(face->glyph_height << 6));

    return FT_Err_Ok;
}

FT_CALLBACK_TABLE_DEF const FT_Driver_ClassRec psf_driver_class = {
    {FT_MODULE_FONT_DRIVER | FT_MODULE_DRIVER_NO_OUTLINES, sizeof(FT_DriverRec), "psf", 0x10000L, 0x20000L, NULL, NULL, NULL, NULL},
    sizeof(PSF_FaceRec),
    sizeof(FT_SizeRec),
    sizeof(FT_GlyphSlotRec),
    psf_face_init,
    psf_face_done,
    NULL,
    NULL,
    NULL,
    NULL,
    psf_glyph_load,
    NULL,
    NULL,
    NULL,
    psf_size_request,
    psf_size_select};
