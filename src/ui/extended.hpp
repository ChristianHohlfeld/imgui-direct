#pragma once
#include "ui/components.hpp"
#include <array>
#include <span>
#include <string>

namespace ui {
// All labels accept ImGui's ## hidden ID suffix. Put repeated items under PushID.
// State belongs to the caller. No retained widget tree, platform or renderer dependency.
struct NavItem { const char* label; bool disabled=false; };
bool Navigation(const char* id, std::span<const NavItem> items, int* selected,
                bool vertical=false, Size size=Size::MD);
bool Breadcrumbs(const char* id, std::span<const char* const> path, int* activated);
bool Pagination(const char* id, int totalItems, int pageSize, int* page);
int page_count(int totalItems, int pageSize);
// Accordion uses -1 for all closed. Returns true only while this section is open.
bool Accordion(const char* label, int index, int* openIndex, bool disabled=false);
bool Collapse(const char* label, bool* open, bool disabled=false);
// EndDropdown/EndContextMenu/EndTable only after a true Begin. EndFieldset always.
bool BeginDropdown(const char* label, Options options={});
void EndDropdown();
bool BeginContextMenu(const char* id);
void EndContextMenu();
bool MenuItem(const char* label, const char* shortcut=nullptr, bool selected=false, bool enabled=true);
void BeginFieldset(const char* id, const char* legend, bool disabled=false);
void EndFieldset();
void Label(const char* text, bool required=false);
void Validation(const char* message, bool valid);
void Validation(const char* message, ValidationState state);
void Divider(const char* label);
void Kbd(const char* shortcut);
void StatusDot(Status status=Status::Info, const char* label=nullptr);
void Avatar(const char* initials, Size size=Size::MD);
void Stat(const char* label, const char* value, const char* description=nullptr);
void Steps(const char* id, std::span<const char* const> labels, int current);
void TimelineItem(const char* title, const char* description, Status status=Status::Info);
void Loading(const char* id, Size size=Size::MD);
void RadialProgress(const char* id, float fraction, Size size=Size::MD, Status status=Status::Info);
void Skeleton(const char* id, ImVec2 size);

struct TableColumn { const char* label; float width=0.f; ImGuiTableColumnFlags flags=0; };
struct TableOptions {
    float height=0.f;
    bool sortable=false;
    ImGuiTableFlags flags=ImGuiTableFlags_RowBg|ImGuiTableFlags_BordersInnerH|
                          ImGuiTableFlags_Resizable|ImGuiTableFlags_SizingStretchProp;
};
bool BeginTable(const char* id, std::span<const TableColumn> columns, TableOptions options={});
void EndTable();
// Sorting is caller-owned. Read ImGui::TableGetSortSpecs() inside Begin/EndTable.
using RowRenderer=void(*)(int row, void* user);
// Fixed-height rows only. Callback must advance exactly rowHeight pixels (including spacing).
// Returns the count submitted, so hosts can measure cost without instrumentation globals.
int VirtualList(const char* id, int count, float height, float rowHeight, RowRenderer render, void* user=nullptr);
// A tree row leaves a TreePush only when true. EndTreeNode exactly once for true.
bool TreeNode(const char* id, const char* label, bool selected=false, bool leaf=false);
void EndTreeNode();
// Two independently scrollable, resizable panes. EndSplit always, even if BeginSplit is false.
// NextSplit switches from first pane to second; call exactly once inside each split.
bool BeginSplit(const char* id, float* fraction, ImVec2 size={0,0}, bool vertical=false, float minPane=80.f);
bool NextSplit();
void EndSplit();

bool Number(const char* label, float* value, float speed=.1f, float min=0.f, float max=0.f, Field field={});
bool Integer(const char* label, int* value, int step=1, Field field={});
bool Vector3(const char* label, float value[3], float speed=.1f, Field field={});
bool ColorEdit(const char* label, float rgba[4], Field field={});
bool Range(const char* label, float* low, float* high, float min, float max, Field field={});
// File selection is an async HOST capability, never a blocking platform dialog in the core.
// Return true means a request was issued, NOT that a file has already arrived.
struct FilePicker { void (*request)(void* user)=nullptr; void* user=nullptr; };
bool FileInput(const char* id, const char* selectedName, FilePicker picker, Field field={});

// Fixed capacity, owned by host; strings allocate only on push, never during draw/update.
// A zero duration stays until dismissed; oldest is evicted on overflow. Time is host supplied.
class ToastQueue {
public:
    static constexpr std::size_t capacity=8;
    struct Entry { std::string message; Status status=Status::Info; double expires=0; unsigned id=0; };
    unsigned push(const char* message, Status status, double now, double duration=4.0);
    bool dismiss(unsigned id);
    void update(double now);
    std::span<const Entry> entries() const { return {items_.data(),count_}; }
private:
    std::array<Entry,capacity> items_{};
    std::size_t count_=0;
    unsigned next_=0;
};
void Toasts(const char* id, ToastQueue& queue, double now);
}
