#ifndef VSPACE_H
#define VSPACE_H
#include "display.h"
#include "space.h"
#include "cursor.h"

#include <unordered_map>
#include "types.h"

void AddVspace(std::string VspaceID);
void DeleteVspace(std::string VspaceID);
void MoveVspaceToDisplay(ax_display *Display, std::string VspaceID);
void MoveWindowToVspace(ax_display *Display, ax_window *Window, std::string VspaceID);


#endif
