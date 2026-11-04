#include "result/ResultStore.h"

#include <cstdlib>
#include <map>

#include "PathUtil.h"
#include "result/Csv.h"

namespace fpe {

namespace {
const char* kExtractHeader = "jobId,kind,image,status,retVal,errCode,errMsg,elapsedUs,size,sehCode\n";
const char* kMatchHeader = "jobId,enroll,attempt,type,status,score,errCode,errMsg,elapsedUs,sehCode\n";

FILE* openAppend(const std::wstring& path, const char* header) {
  FILE* f = _wfopen(path.c_str(), L"ab");
  if (!f) return nullptr;
  _fseeki64(f, 0, SEEK_END);
  if (_ftelli64(f) == 0) {  // 새 파일이거나 머리글을 쓰기 전에 강제 종료된 빈 파일
    fputs(header, f);
    fflush(f);
  }
  return f;
}

std::string hex(uint32_t v) {
  if (!v) return {};
  char b[16];
  snprintf(b, sizeof b, "0x%08X", v);
  return b;
}

std::string fmtScore(double s) {
  char b[32];
  snprintf(b, sizeof b, "%.6g", s);
  return b;
}
}  // namespace

ResultStore::~ResultStore() { close(); }

bool ResultStore::open(const std::wstring& runDir, std::string& err) {
  close();
  fe_ = openAppend(joinPath(runDir, L"log_enroll.csv"), kExtractHeader);
  fm_ = openAppend(joinPath(runDir, L"log_match.csv"), kMatchHeader);
  if (!fe_ || !fm_) {
    err = "cannot open log files";
    close();
    return false;
  }
  return true;
}

void ResultStore::close() {
  std::lock_guard<std::mutex> lk(mu_);
  if (fe_) fclose(fe_);
  if (fm_) fclose(fm_);
  fe_ = fm_ = nullptr;
}

void ResultStore::append(const ExtractRecord& r) {
  std::string line = csvRow({std::to_string(r.jobId), std::string(1, r.kind), r.image, toString(r.status),
                             std::to_string(r.retVal), std::to_string(r.errCode), r.errMsg,
                             std::to_string(r.elapsedUs), r.status == Status::OK ? std::to_string(r.size) : "",
                             hex(r.sehCode)});
  std::lock_guard<std::mutex> lk(mu_);
  if (!fe_) return;
  fputs(line.c_str(), fe_);
  fflush(fe_);
}

void ResultStore::append(const MatchRecord& r) {
  bool called = r.status != Status::SkipEF && r.status != Status::SkipAF;
  std::string line =
      csvRow({std::to_string(r.jobId), r.enroll, r.attempt, std::string(1, r.type), toString(r.status),
              r.status == Status::OK || r.status == Status::Mismatch ? fmtScore(r.score) : "",
              called ? std::to_string(r.errCode) : "", r.errMsg, called ? std::to_string(r.elapsedUs) : "",
              hex(r.sehCode)});
  std::lock_guard<std::mutex> lk(mu_);
  if (!fm_) return;
  fputs(line.c_str(), fm_);
  fflush(fm_);
}

bool ResultStore::load(const std::wstring& runDir, std::vector<ExtractRecord>& ex, std::vector<MatchRecord>& mt,
                       std::string& err) {
  ex.clear();
  mt.clear();
  std::vector<uint8_t> buf;
  std::map<uint32_t, ExtractRecord> em;
  std::map<uint32_t, MatchRecord> mm;
  auto toI = [](const std::string& s) { return s.empty() ? 0 : atoi(s.c_str()); };
  auto toL = [](const std::string& s) { return s.empty() ? 0LL : _atoi64(s.c_str()); };
  auto toU = [](const std::string& s) { return s.empty() ? 0u : (uint32_t)strtoul(s.c_str(), nullptr, 0); };

  if (readFile(joinPath(runDir, L"log_enroll.csv"), buf)) {
    for (const auto& line : splitLines(std::string(buf.begin(), buf.end()))) {
      if (line.rfind("jobId", 0) == 0) continue;
      auto f = parseCsvLine(line);
      if (f.size() < 10) continue;
      ExtractRecord r;
      r.jobId = toU(f[0]);
      r.kind = f[1].empty() ? 'E' : f[1][0];
      r.image = f[2];
      r.status = statusFromString(f[3]);
      r.retVal = toI(f[4]);
      r.errCode = toI(f[5]);
      r.errMsg = f[6];
      r.elapsedUs = toL(f[7]);
      r.size = toI(f[8]);
      r.sehCode = toU(f[9]);
      em[r.jobId] = r;
    }
  }
  if (readFile(joinPath(runDir, L"log_match.csv"), buf)) {
    for (const auto& line : splitLines(std::string(buf.begin(), buf.end()))) {
      if (line.rfind("jobId", 0) == 0) continue;
      auto f = parseCsvLine(line);
      if (f.size() < 10) continue;
      MatchRecord r;
      r.jobId = toU(f[0]);
      r.enroll = f[1];
      r.attempt = f[2];
      r.type = f[3].empty() ? 'G' : f[3][0];
      r.status = statusFromString(f[4]);
      r.score = f[5].empty() ? 0.0 : atof(f[5].c_str());
      r.errCode = toI(f[6]);
      r.errMsg = f[7];
      r.elapsedUs = toL(f[8]);
      r.sehCode = toU(f[9]);
      mm[r.jobId] = r;
    }
  }
  for (auto& kv : em) ex.push_back(kv.second);
  for (auto& kv : mm) mt.push_back(kv.second);
  if (ex.empty() && mt.empty()) {
    err = "no log records in " + toUtf8(runDir);
    return false;
  }
  return true;
}

}  // namespace fpe
