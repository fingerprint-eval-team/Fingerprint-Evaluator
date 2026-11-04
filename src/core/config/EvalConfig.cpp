#include "config/EvalConfig.h"

#include "PathUtil.h"

namespace fpe {

std::vector<std::string> allReportItems() {
  return {item::FTE,    item::REJ,  item::EER,  item::EERstar, item::ZeroFMR, item::ZeroFNMR, item::FMR100,
          item::FMR1000, item::Time, item::Size, item::DET,     item::Dist,    item::Rates,    item::Errors};
}

Json EvalConfig::toJson() const {
  Json j = Json::object();
  j["dllPath"] = toUtf8(dllPath);
  Json& db = j["db"];
  db["path"] = toUtf8(dbPath);
  db["iType"] = iType;
  j["templateDir"] = toUtf8(templateDir);
  j["outputDir"] = toUtf8(outputDir);

  Json& r = j["rules"];
  r["enroll"] = enrollRule;
  r["enrollIndex"] = enrollIndex;
  r["genuine"] = genuineRule;
  r["genuineN"] = genuineN;
  r["impostor"] = impostorRule;
  r["impostorN"] = impostorN;
  Json idx = Json::array();
  for (int i : impostorIndices) idx.push(i);
  r["impostorIndices"] = idx;
  r["impostorSymmetric"] = impostorSymmetric;
  r["pairsFile"] = toUtf8(pairsFile);
  r["seed"] = (long long)seed;

  Json& run = j["run"];
  run["workers"] = workers;
  run["timeoutSec"] = timeoutSec;
  run["anonymizeNames"] = anonymize;

  Json& rep = j["report"];
  Json fmts = Json::array();
  if (html) fmts.push("html");
  if (docx) fmts.push("docx");
  rep["formats"] = fmts;
  Json it = Json::array();
  for (auto& s : items) it.push(s);
  rep["items"] = it;
  rep["eerMethod"] = eerMethod;
  rep["denominator"] = denominator;
  return j;
}

EvalConfig EvalConfig::fromJson(const Json& j) {
  EvalConfig c;
  c.dllPath = fromUtf8(j.get("dllPath").asString());
  c.dbPath = fromUtf8(j.get("db").get("path").asString());
  c.iType = j.get("db").get("iType").asInt(1);
  c.templateDir = fromUtf8(j.get("templateDir").asString());
  c.outputDir = fromUtf8(j.get("outputDir").asString());

  const Json& r = j.get("rules");
  c.enrollRule = r.get("enroll").asString(c.enrollRule);
  c.enrollIndex = r.get("enrollIndex").asInt(c.enrollIndex);
  c.genuineRule = r.get("genuine").asString(c.genuineRule);
  c.genuineN = r.get("genuineN").asInt(c.genuineN);
  c.impostorRule = r.get("impostor").asString(c.impostorRule);
  c.impostorN = r.get("impostorN").asInt(c.impostorN);
  if (r.has("impostorIndices")) {
    c.impostorIndices.clear();
    const Json& a = r.get("impostorIndices");
    for (size_t i = 0; i < a.size(); ++i) c.impostorIndices.push_back(a.at(i).asInt());
  }
  c.impostorSymmetric = r.get("impostorSymmetric").asBool(c.impostorSymmetric);
  c.pairsFile = fromUtf8(r.get("pairsFile").asString());
  c.seed = (uint64_t)r.get("seed").asNumber((double)c.seed);

  const Json& run = j.get("run");
  c.workers = run.get("workers").asInt(c.workers);
  c.timeoutSec = run.get("timeoutSec").asInt(c.timeoutSec);
  c.anonymize = run.get("anonymizeNames").asBool(c.anonymize);

  const Json& rep = j.get("report");
  if (rep.has("formats")) {
    c.html = c.docx = false;
    const Json& f = rep.get("formats");
    for (size_t i = 0; i < f.size(); ++i) {
      if (f.at(i).asString() == "html") c.html = true;
      if (f.at(i).asString() == "docx") c.docx = true;
    }
  }
  if (rep.has("items")) {
    const Json& it = rep.get("items");
    for (size_t i = 0; i < it.size(); ++i) c.items.insert(it.at(i).asString());
  }
  if (c.items.empty())
    for (auto& s : allReportItems()) c.items.insert(s);
  c.eerMethod = rep.get("eerMethod").asString(c.eerMethod);
  c.denominator = rep.get("denominator").asString(c.denominator);
  return c;
}

bool EvalConfig::save(const std::wstring& path) const { return writeText(path, toJson().dump(2)); }

bool EvalConfig::load(const std::wstring& path, EvalConfig& out, std::string& err) {
  std::vector<uint8_t> buf;
  if (!readFile(path, buf)) {
    err = "cannot read " + toUtf8(path);
    return false;
  }
  Json j;
  if (!Json::parse(std::string(buf.begin(), buf.end()), j, err)) return false;
  out = fromJson(j);
  return true;
}

}  // namespace fpe
