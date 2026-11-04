// 최소 OOXML(WordprocessingML) 문서 생성: 제목·표·EMF 그림
#include "PathUtil.h"
#include "report/ReportModel.h"
#include "report/ZipWriter.h"

namespace fpe {

namespace {
std::string x(const std::string& s) {
  std::string o;
  for (char c : s) {
    switch (c) {
      case '<': o += "&lt;"; break;
      case '>': o += "&gt;"; break;
      case '&': o += "&amp;"; break;
      case '"': o += "&quot;"; break;
      default: o += c;
    }
  }
  return o;
}

std::string run(const std::string& text, int halfPts, bool bold, const char* color = nullptr) {
  std::string r = "<w:r><w:rPr><w:rFonts w:ascii=\"Segoe UI\" w:hAnsi=\"Segoe UI\" w:eastAsia=\"맑은 고딕\"/>";
  if (bold) r += "<w:b/>";
  if (color) r += std::string("<w:color w:val=\"") + color + "\"/>";
  r += "<w:sz w:val=\"" + std::to_string(halfPts) + "\"/></w:rPr><w:t xml:space=\"preserve\">" + x(text) +
       "</w:t></w:r>";
  return r;
}
}  // namespace

void DocxReportWriter::begin(const ReportModel&) {
  body_.clear();
  images_.clear();
}

void DocxReportWriter::heading(int level, const std::string& text) {
  body_ += "<w:p><w:pPr><w:spacing w:before=\"" + std::string(level == 1 ? "0" : "320") +
           "\" w:after=\"120\"/></w:pPr>" + run(text, level == 1 ? 36 : 28, true, level == 1 ? "1F4FA0" : nullptr) +
           "</w:p>";
}

void DocxReportWriter::paragraph(const std::string& text) { body_ += "<w:p>" + run(text, 20, false) + "</w:p>"; }

void DocxReportWriter::table(const std::vector<std::vector<std::string>>& rows, bool headerRow) {
  body_ +=
      "<w:tbl><w:tblPr><w:tblW w:w=\"5000\" w:type=\"pct\"/><w:tblBorders>"
      "<w:top w:val=\"single\" w:sz=\"4\" w:color=\"C8CED8\"/><w:left w:val=\"single\" w:sz=\"4\" w:color=\"C8CED8\"/>"
      "<w:bottom w:val=\"single\" w:sz=\"4\" w:color=\"C8CED8\"/><w:right w:val=\"single\" w:sz=\"4\" w:color=\"C8CED8\"/>"
      "<w:insideH w:val=\"single\" w:sz=\"4\" w:color=\"C8CED8\"/><w:insideV w:val=\"single\" w:sz=\"4\" w:color=\"C8CED8\"/>"
      "</w:tblBorders><w:tblCellMar><w:left w:w=\"100\" w:type=\"dxa\"/><w:right w:w=\"100\" w:type=\"dxa\"/>"
      "</w:tblCellMar></w:tblPr>";
  for (size_t r = 0; r < rows.size(); ++r) {
    bool hdr = headerRow && r == 0;
    body_ += "<w:tr>";
    for (auto& c : rows[r]) {
      body_ += "<w:tc><w:tcPr>";
      if (hdr) body_ += "<w:shd w:val=\"clear\" w:color=\"auto\" w:fill=\"EEF1F5\"/>";
      body_ += "</w:tcPr><w:p><w:pPr><w:spacing w:before=\"40\" w:after=\"40\"/></w:pPr>" + run(c, 18, hdr) +
               "</w:p></w:tc>";
    }
    body_ += "</w:tr>";
  }
  body_ += "</w:tbl><w:p/>";
}

void DocxReportWriter::figure(const ReportFigure& f) {
  images_.push_back(f);
  size_t n = images_.size();
  std::string rid = "rIdImg" + std::to_string(n);
  // 640x480 px → 15cm x 11.25cm (EMU)
  const long long cx = 5400000, cy = 4050000;
  body_ += "<w:p><w:pPr><w:jc w:val=\"center\"/></w:pPr><w:r><w:drawing><wp:inline distT=\"0\" distB=\"0\" "
           "distL=\"0\" distR=\"0\"><wp:extent cx=\"" + std::to_string(cx) + "\" cy=\"" + std::to_string(cy) +
           "\"/><wp:docPr id=\"" + std::to_string(n) + "\" name=\"" + x(f.id) +
           "\"/><a:graphic xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\"><a:graphicData "
           "uri=\"http://schemas.openxmlformats.org/drawingml/2006/picture\"><pic:pic "
           "xmlns:pic=\"http://schemas.openxmlformats.org/drawingml/2006/picture\"><pic:nvPicPr><pic:cNvPr id=\"" +
           std::to_string(n) + "\" name=\"" + x(f.id) + ".emf\"/><pic:cNvPicPr/></pic:nvPicPr><pic:blipFill><a:blip "
           "r:embed=\"" + rid + "\"/><a:stretch><a:fillRect/></a:stretch></pic:blipFill><pic:spPr><a:xfrm><a:off "
           "x=\"0\" y=\"0\"/><a:ext cx=\"" + std::to_string(cx) + "\" cy=\"" + std::to_string(cy) +
           "\"/></a:xfrm><a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom></pic:spPr></pic:pic></a:graphicData>"
           "</a:graphic></wp:inline></w:drawing></w:r></w:p>";
  body_ += "<w:p><w:pPr><w:jc w:val=\"center\"/></w:pPr>" + run(f.caption, 18, false, "5A6475") + "</w:p>";
}

bool DocxReportWriter::end(const std::wstring& path, std::string& err) {
  ZipWriter z;
  z.add("[Content_Types].xml",
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
        "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
        "<Default Extension=\"emf\" ContentType=\"image/x-emf\"/>"
        "<Override PartName=\"/word/document.xml\" "
        "ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml\"/>"
        "</Types>");
  z.add("_rels/.rels",
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" "
        "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" "
        "Target=\"word/document.xml\"/></Relationships>");

  std::string rels =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
      "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">";
  for (size_t i = 0; i < images_.size(); ++i) {
    std::vector<uint8_t> data;
    if (!readFile(images_[i].emfPath, data)) {
      err = "cannot read " + toUtf8(images_[i].emfPath);
      return false;
    }
    std::string media = "media/" + images_[i].id + ".emf";
    z.add("word/" + media, std::move(data));
    rels += "<Relationship Id=\"rIdImg" + std::to_string(i + 1) +
            "\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\" Target=\"" + media +
            "\"/>";
  }
  rels += "</Relationships>";
  z.add("word/_rels/document.xml.rels", rels);

  std::string doc =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
      "<w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\" "
      "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
      "xmlns:wp=\"http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing\"><w:body>" +
      body_ +
      "<w:sectPr><w:pgSz w:w=\"11906\" w:h=\"16838\"/><w:pgMar w:top=\"1134\" w:right=\"1134\" w:bottom=\"1134\" "
      "w:left=\"1134\" w:header=\"567\" w:footer=\"567\" w:gutter=\"0\"/></w:sectPr></w:body></w:document>";
  z.add("word/document.xml", doc);

  auto bytes = z.finish();
  if (!writeFile(path, bytes.data(), bytes.size())) {
    err = "cannot write " + toUtf8(path);
    return false;
  }
  return true;
}

}  // namespace fpe
