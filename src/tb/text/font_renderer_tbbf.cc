/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <algorithm>
#include <memory>
#include <unordered_map>

#include "tb/graphics/image_loader.h"
#include "tb/graphics/renderer.h"
#include "tb/parsing/parse_node.h"
#include "tb/text/font_face.h"
#include "tb/text/font_manager.h"
#include "tb/text/font_renderer.h"
#include "tb/text/utf8.h"

#ifdef TB_FONT_RENDERER_TBBF

namespace tb {
namespace text {

using namespace tb::parsing;

namespace {
struct GLYPH {
  int x, w;
};
}  // namespace

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
  std::unique_ptr<GLYPH> FindNext(UCS4 cp, int x);

  std::unique_ptr<FontFace> Create(FontManager* font_manager,
                                   const std::string& filename,
                                   const FontDescription& font_desc) override;

  FontMetrics GetMetrics() override;
  bool RenderGlyph(FontGlyphData* dst_bitmap, UCS4 cp) override;
  void GetGlyphMetrics(GlyphMetrics* metrics, UCS4 cp) override;

 private:
  ParseNode m_node;
  FontMetrics m_metrics;
  std::unique_ptr<graphics::ImageLoader> m_img;
  int m_size;
  int m_x_ofs;
  int m_advance_delta;
  int m_space_advance;
  int m_rgb;
  std::unordered_map<uint32_t, std::unique_ptr<GLYPH>> m_glyph_table;
};

TBBFRenderer::TBBFRenderer()
    : m_size(0), m_x_ofs(0), m_advance_delta(0), m_space_advance(0), m_rgb(0) {}

TBBFRenderer::~TBBFRenderer() = default;

FontMetrics TBBFRenderer::GetMetrics() { return m_metrics; }

bool TBBFRenderer::RenderGlyph(FontGlyphData* data, UCS4 cp) {
  if (cp == ' ') {
    return false;
  }

  auto& it = m_glyph_table.find(cp);
  if (it == m_glyph_table.end()) {
    it = m_glyph_table.find('?');
  }
  if (it == m_glyph_table.end()) {
    return false;
  }
  auto glyph = it->second.get();
  data->w = glyph->w;
  data->h = m_img->height();
  data->stride = m_img->width();
  data->data32 = m_img->data() + glyph->x;
  data->rgb = m_rgb ? true : false;
  return true;
}

void TBBFRenderer::GetGlyphMetrics(GlyphMetrics* metrics, UCS4 cp) {
  metrics->x = m_x_ofs;
  metrics->y = -m_metrics.ascent;
  if (cp == ' ') {
    metrics->advance = m_space_advance;
  } else {
    auto& it = m_glyph_table.find(cp);
    if (it != m_glyph_table.end()) {
      metrics->advance = it->second->w + m_advance_delta;
    } else {
      it = m_glyph_table.find('?');
      if (it != m_glyph_table.end()) {
        metrics->advance = it->second->w + m_advance_delta;
      } else {
        metrics->advance = 0;
      }
    }
  }
}

bool TBBFRenderer::Load(const std::string& filename, int size) {
  m_size = size;
  if (!m_node.ReadFile(filename)) return false;

  // Check for size nodes and get the one closest to the size we want.
  ParseNode* size_node = nullptr;
  for (ParseNode* n = m_node.first_child(); n; n = n->GetNext()) {
    if (strcmp(n->name(), "size") == 0) {
      if (!size_node ||
          std::abs(m_size - n->value().as_integer()) <
              std::abs(m_size - size_node->value().as_integer()))
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

  m_img = graphics::ImageLoader::CreateFromFile(bitmap_filename.c_str());

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
  while (UCS4 uc = utf8::decode_next(glyph_str, &i, glyph_str_len)) {
    auto glyph = FindNext(uc, x);
    if (!glyph) {
      break;
    }
    x = glyph->x + glyph->w + 1;
    m_glyph_table.emplace(uc, std::move(glyph));
  }
  return true;
}

std::unique_ptr<GLYPH> TBBFRenderer::FindNext(UCS4 cp, int x) {
  int width = m_img->width();
  int height = m_img->height();
  uint32_t* data32 = m_img->data();

  if (x >= width) {
    return nullptr;
  }

  auto glyph = std::make_unique<GLYPH>();
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
      if (GetAlpha(data32[i + j * width])) {
        break;
      }
    }
    if (j == height) {
      // The whole col was clear, so we found the edge
      glyph->w = i - glyph->x;
      break;
    }
  }

  if (glyph->x == -1 || glyph->w == -1) {
    return nullptr;
  }
  return glyph;
}

std::unique_ptr<FontFace> TBBFRenderer::Create(
    FontManager* font_manager, const std::string& filename,
    const FontDescription& font_desc) {
  if (!strstr(filename.c_str(), ".tb.txt")) return nullptr;
  auto fr = std::make_unique<TBBFRenderer>();
  if (!fr->Load(filename, (int)font_desc.size())) {
    return nullptr;
  }
  return std::make_unique<FontFace>(font_manager->glyph_cache(), std::move(fr),
                                    font_desc);
}

}  // namespace text
}  // namespace tb

void register_tbbf_font_renderer() {
  tb::text::FontManager::get()->RegisterRenderer(
      std::make_unique<tb::text::TBBFRenderer>());
}

#endif  // TB_FONT_RENDERER_TBBF
