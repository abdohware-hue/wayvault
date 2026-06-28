#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

namespace wayvault {

struct SearchHit {
  std::string document_id;
  double score = 0.0;
  std::vector<std::string> matched_terms;
};

class TextIndex {
 public:
  void add_document(const std::string& id, const std::string& text);
  void remove_document(const std::string& id);
  std::vector<SearchHit> search(const std::string& query, size_t limit) const;
  size_t document_count() const { return documents_.size(); }
  size_t term_count() const { return postings_.size(); }

 private:
  std::map<std::string, std::string> documents_;
  std::map<std::string, std::set<std::string>> postings_;
};

std::vector<std::string> tokenize_text(const std::string& text);
std::set<std::string> trigrams_for_text(const std::string& text);
double trigram_similarity(const std::string& a, const std::string& b);
std::string normalize_search_text(const std::string& text);

}  // namespace wayvault
