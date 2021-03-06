#include "History.hpp"

#include <libpx.hpp>

#include <vector>

namespace px {

class HistoryImpl final
{
  friend History;

  std::vector<Document*> snapshots;
  std::size_t pos = 0;
  std::size_t saved = 0;

  ~HistoryImpl();
};

History::History(Document* doc) : impl(new HistoryImpl())
{
  if (doc) {
    impl->snapshots.emplace_back(doc);
  } else {
    impl->snapshots.emplace_back(createDoc());
  }
}

History::~History()
{
  delete impl;
}

Document* History::getDocument() noexcept
{
  return impl->snapshots[impl->pos];
}

const Document* History::getDocument() const noexcept
{
  return impl->snapshots[impl->pos];
}

int History::open(const char* path, ErrorList** errList)
{
  delete impl;

  impl = new HistoryImpl();

  return openDoc(impl->snapshots[0], path, errList);
}

void History::snapshot()
{
  impl->snapshots.resize(impl->pos + 1);

  impl->snapshots.emplace_back(copyDoc(getDocument()));

  impl->pos = impl->snapshots.size() - 1;
}

void History::undo()
{
  if (impl->pos > 0) {
    impl->pos--;
  }
}

void History::redo()
{
  if ((impl->pos + 1) < impl->snapshots.size()) {
    impl->pos++;
  }
}

void History::markSaved()
{
  impl->saved = impl->pos;
}

bool History::isSaved() const noexcept
{
  return impl->pos == impl->saved;
}

History& History::operator = (History&& other) noexcept
{
  delete impl;
  impl = other.impl;
  other.impl = nullptr;
  return *this;
}

HistoryImpl::~HistoryImpl()
{
  for (auto* doc : snapshots) {
    closeDoc(doc);
  }
};

} // namespace px
