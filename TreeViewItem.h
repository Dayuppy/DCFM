#ifndef TREEVIEWITEM_H
#define TREEVIEWITEM_H

#include <string>
#include <vector>
#include <memory>

class TreeViewItem {
public:
    std::wstring Header;
    std::wstring Tag;
    std::vector<std::shared_ptr<TreeViewItem>> Items;
};

#endif // TREEVIEWITEM_H