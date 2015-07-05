/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <algorithm>

#include "tb/graphics/image_loader.h"
#include "tb/graphics/renderer.h"
#include "tb/parsing/parse_node.h"
#include "tb/resources/font_face.h"
#include "tb/resources/font_manager.h"
#include "tb/resources/font_renderer.h"

#ifdef TB_FONT_RENDERER_TBBF

using namespace tb;
using namespace tb::parsing;
using namespace tb::resources;

struct GLYPH {
  int x, w;
};

/** TBBFRenderer renders a bitmap font.

        A font is loaded from a text file and at least one image that contains
        glyphs for a given size. The number of glyphs that the font contains is
        defined by the glyph string defined in the text file.

        Text file format (in tb.txt format parsed by parser/tb_parser.h):

                - info>glyph_str			Should specify which
   characters the image
                                                                file contains.
                - info>rgb				Set to 1 for color
   fonts that should never
                                                                care about the
   text color when drawing.
                                                                Set to 0 to let
   drawing blend using the text
                                                                color. Default
   0.
                - size xx				Specify font size xx.
   Should contain the
                                                                following nodes:
                        - bitmap				The image file
   name (in the same folder).
                        - ascent				The ascent.
   Default 0.
                        - descent			The descent. Default 0.
                        - x_ofs				The x offset for
   all glyph. This can be
                                                                used in
   combination with advance_delta to
                                                                compensate for
   f.ex glow that extend
                                                                around the
   glyph.  Default 0.
                        - advance_delta		The advance delta for all
   glyphs. This can
                                                                be used to
   compensate for f.ex shadow that
                                                                should not add
   to each glyphs horizontal
                                                                advance. Default
   0.
                        - space_advance		The advance for the space
   character.

        Image file format

                Should contain the characters specified in the glyph_str.

                All characters should be placed on one long line. Each glyph
   will be
                found, measured and cropped automatically. In order for this to
   work,
                each glyph must touch pixels somewhere from the left to the
   right edge.
                So if you f.ex have a quotation mark, you will have to make sure
   there
                is pixels with alpha > 0 between the two dots, otherwise the
   dots will
                be identified as different glyphs.
*/
class TBBFRenderer : public FontRenderer {
 public:
  TBBFRenderer();
  ~TBBFRenderer();

  bool Load(const std::string& filename, int size);
  bool FindGlyphs();
  GLYPH* FindNext(UCS4 cp, int x);

  virtual FontFace* Create(FontManager* font_manager,
                           const std::string& filename,
                           const FontDescription& font_desc);

  virtual FontMetrics GetMetrics();
  virtual bool RenderGlyph(FontGlyphData* dst_bitmap, UCS4 cp);
  virtual void GetGlyphMetrics(GlyphMetrics* metrics, UCS4 cp);

 private:
  ParseNode m_node;
  FontMetrics m_metrics;
  graphics::ImageLoader* m_img;
  int m_size;
  int m_x_ofs;
  int m_advance_delta;
  int m_space_advance;
  int m_rgb;
  util::HashTableAutoDeleteOf<GLYPH> m_glyph_table;
};

TBBFRenderer::TBBFRenderer()
    : m_img(nullptr),
      m_size(0),
      m_x_ofs(0),
      m_advance_delta(0),
      m_space_advance(0),
      m_rgb(0) {}

TBBFRenderer::~TBBFRenderer() { delete m_img; }

FontMetrics TBBFRenderer::GetMetrics() { return m_metrics; }

bool TBBFRenderer::RenderGlyph(FontGlyphData* data, UCS4 cp) {
  if (cp == ' ') return false;
  GLYPH* glyph;
  if ((glyph = m_glyph_table.Get(cp)) || (glyph = m_glyph_table.Get('?'))) {
    data->w = glyph->w;
    data->h = m_img->Height();
    data->stride = m_img->Width();
    data->data32 = m_img->Data() + glyph->x;
    data->rgb = m_rgb ? true : false;
    return true;
  }
  return false;
}

void TBBFRenderer::GetGlyphMetrics(GlyphMetrics* metrics, UCS4 cp) {
  metrics->x = m_x_ofs;
  metrics->y = -m_metrics.ascent;
  if (cp == ' ')
    metrics->advance = m_space_advance;
  else if (GLYPH* glyph = m_glyph_table.Get(cp))
    metrics->advance = glyph->w + m_advance_delta;
  else if (GLYPH* glyph = m_glyph_table.Get('?'))
    metrics->advance = glyph->w + m_advance_delta;
}

bool TBBFRenderer::Load(const std::string& filename, int size) {
  m_size = size;
  if (!m_node.ReadFile(filename)) return false;

  // Check for size nodes and get the one closest to the size we want.
  ParseNode* size_node = nullptr;
  for (ParseNode* n = m_node.GetFirstChild(); n; n = n->GetNext()) {
    if (strcmp(n->GetName(), "size") == 0) {
      if (!size_node ||
          std::abs(m_size - n->GetValue().as_integer()) <
              std::abs(m_size - size_node->GetValue().as_integer()))
        size_node = n;
    }
  }
  if (!size_node) return false;

  // Metrics
  m_metrics.ascent = size_node->GetValueInt("ascent", 0);
  m_metrics.descent = size_node->GetValueInt("descent", 0);
  m_metrics.height = m_metrics.ascent + m_metrics.descent;

  // Other data
  m_advance_delta = size_node->GetValueInt("advance_delta", 0);
  m_space_advance = size_node->GetValueInt("space_advance", 0);
  m_x_ofs = size_node->GetValueInt("x_ofs", 0);

  // Info
  m_rgb = m_node.GetValueInt("info>rgb", 0);

  // Get the path for the bitmap file.
  util::StringBuilder bitmap_filename;
  bitmap_filename.AppendPath(filename);

  // Append the bitmap filename for the given size.
  bitmap_filename.AppendString(size_node->GetValueString("bitmap", ""));

  m_img = graphics::ImageLoader::CreateFromFile(bitmap_filename.GetData());

  return FindGlyphs();
}

inline unsigned char GetAlpha(uint32_t color) {
  return (color & 0xff000000) >> 24;
}

bool TBBFRenderer::FindGlyphs() {
  if (!m_img) return false;

  const char* glyph_str = m_node.GetValueString("info>glyph_str", nullptr);
  if (!glyph_str) return false;

  size_t glyph_str_len = strlen(glyph_str);
  size_t i = 0;
  int x = 0;
  while (UCS4 uc = util::utf8::decode_next(glyph_str, &i, glyph_str_len)) {
    if (GLYPH* glyph = FindNext(uc, x)) {
      m_glyph_table.Add(uc, glyph);
      x = glyph->x + glyph->w + 1;
    } else
      break;
  }
  return true;
}

GLYPH* TBBFRenderer::FindNext(UCS4 cp, int x) {
  int width = m_img->Width();
  int height = m_img->Height();
  uint32_t* data32 = m_img->Data();

  if (x >= width) return nullptr;

  GLYPH* glyph = new GLYPH();

  glyph->x = -1;
  glyph->w = -1;

  // Find the left edge of the glyph
  for (int i = x; i < width && glyph->x == -1; ++i) {
    for (int j = 0; j < height; j++)
      if (GetAlpha(data32[i + j * width])) {
        glyph->x = x = i;
        break;
      }
  }

  // Find the right edge of the glyph
  for (int i = x; i < width; ++i) {
    int j;
    for (j = 0; j < height; j++) {
      if (GetAlpha(data32[i + j * width])) break;
    }
    if (j == height)  // The whole col was clear, so we found the edge
    {
      glyph->w = i - glyph->x;
      break;
    }
  }

  if (glyph->x == -1 || glyph->w == -1) {
    delete glyph;
    return nullptr;
  }
  return glyph;
}

FontFace* TBBFRenderer::Create(FontManager* font_manager,
                               const std::string& filename,
                               const FontDescription& font_desc) {
  if (!strstr(filename.c_str(), ".tb.txt")) return nullptr;
  TBBFRenderer* fr = new TBBFRenderer();
  if (fr->Load(filename, (int)font_desc.GetSize())) {
    FontFace* font = new FontFace(font_manager->GetGlyphCache(), fr, font_desc);
    return font;
  }
  delete fr;
  return nullptr;
}

void register_tbbf_font_renderer() {
  TBBFRenderer* fr = new TBBFRenderer();
  FontManager::get()->AddRenderer(fr);
}

#endif  // TB_FONT_RENDERER_TBBF
