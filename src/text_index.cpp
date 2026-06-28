#include "wayvault/text_index.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <set>

namespace wayvault {

std::string normalize_search_text(const std::string& text) {
  std::string out;
  bool last_space = true;
  for (unsigned char ch : text) {
    if (std::isalnum(ch)) {
      out.push_back(static_cast<char>(std::tolower(ch)));
      last_space = false;
    } else if (!last_space) {
      out.push_back(' ');
      last_space = true;
    }
  }
  if (!out.empty() && out.back() == ' ') out.pop_back();
  return out;
}

std::vector<std::string> tokenize_text(const std::string& text) {
  std::vector<std::string> tokens;
  std::string normalized = normalize_search_text(text);
  size_t start = 0;
  for (size_t i = 0; i <= normalized.size(); ++i) {
    if (i == normalized.size() || normalized[i] == ' ') {
      if (i > start) tokens.push_back(normalized.substr(start, i - start));
      start = i + 1;
    }
  }
  return tokens;
}

std::set<std::string> trigrams_for_text(const std::string& text) {
  std::string s = "  " + normalize_search_text(text) + "  ";
  std::set<std::string> grams;
  if (s.size() < 3) return grams;
  for (size_t i = 0; i + 3 <= s.size(); ++i) grams.insert(s.substr(i, 3));
  return grams;
}

double trigram_similarity(const std::string& a, const std::string& b) {
  std::set<std::string> ga = trigrams_for_text(a);
  std::set<std::string> gb = trigrams_for_text(b);
  if (ga.empty() && gb.empty()) return 1.0;
  size_t intersection = 0;
  for (const std::string& gram : ga) if (gb.count(gram)) ++intersection;
  size_t uni = ga.size() + gb.size() - intersection;
  return uni == 0 ? 0.0 : static_cast<double>(intersection) / static_cast<double>(uni);
}

void TextIndex::add_document(const std::string& id, const std::string& text) {
  remove_document(id);
  documents_[id] = text;
  std::set<std::string> unique_terms;
  for (const std::string& token : tokenize_text(text)) unique_terms.insert(token);
  for (const std::string& term : unique_terms) postings_[term].insert(id);
}

void TextIndex::remove_document(const std::string& id) {
  auto existing = documents_.find(id);
  if (existing == documents_.end()) return;
  for (const std::string& token : tokenize_text(existing->second)) {
    auto it = postings_.find(token);
    if (it == postings_.end()) continue;
    it->second.erase(id);
    if (it->second.empty()) postings_.erase(it);
  }
  documents_.erase(existing);
}

std::vector<SearchHit> TextIndex::search(const std::string& query, size_t limit) const {
  std::map<std::string, SearchHit> hits;
  std::vector<std::string> terms = tokenize_text(query);
  for (const std::string& term : terms) {
    auto exact = postings_.find(term);
    if (exact != postings_.end()) {
      for (const std::string& id : exact->second) {
        SearchHit& hit = hits[id];
        hit.document_id = id;
        hit.score += 2.0;
        hit.matched_terms.push_back(term);
      }
    }
    for (const auto& kv : postings_) {
      if (kv.first == term) continue;
      double sim = trigram_similarity(term, kv.first);
      if (sim < 0.45) continue;
      for (const std::string& id : kv.second) {
        SearchHit& hit = hits[id];
        hit.document_id = id;
        hit.score += sim;
        hit.matched_terms.push_back(kv.first);
      }
    }
  }
  for (auto& kv : hits) {
    auto doc = documents_.find(kv.first);
    if (doc != documents_.end()) kv.second.score += 0.1 * trigram_similarity(query, doc->second);
  }
  std::vector<SearchHit> out;
  for (auto& kv : hits) out.push_back(std::move(kv.second));
  std::stable_sort(out.begin(), out.end(), [](const SearchHit& a, const SearchHit& b) {
    if (std::abs(a.score - b.score) > 1e-9) return a.score > b.score;
    return a.document_id < b.document_id;
  });
  if (limit != 0 && out.size() > limit) out.resize(limit);
  return out;
}

}  // namespace wayvault
