#include "DrawTool.hpp"

#include "Editor.hpp"

namespace px {

Document* DrawTool::getDocument() noexcept
{
  return editor->getDocument();
}

const Document* DrawTool::getDocument() const noexcept
{
  return editor->getDocument();
}

} // namespace px