#include "vspace.h"
#define UNASSIGNED -1
#define NOSPACEAVAILABLE -1

std::map<std::string, space_identifier> VspaceToSpaceIdentifier;
std::map<space_identifier, std::string> SpaceIdentifierToVspace;


enum vspace_move_type {
    VspaceNoMove,
    VspaceNewSpace,
    VspaceSameScreenMove,
    VspaceDifferentScreenMove
};


vspace_move_type getMoveType(space_identifier &PreviousSpace, space_identifier &ActiveSpace) {
    if (PreviousSpace.ScreenID == UNASSIGNED || PreviousSpace.SpaceID == UNASSIGNED ||
        ActiveSpace.ScreenID == UNASSIGNED || ActiveSpace.SpaceID == UNASSIGNED) {
        return VspaceNewSpace;
    }
    if (PreviousSpace.ScreenID == ActiveSpace.ScreenID) {
        if (PreviousSpace.SpaceID == ActiveSpace.SpaceID) {
            return VspaceNoMove;
        } else {
            return VspaceSameScreenMove;
        }
    } else {
        return VspaceDifferentScreenMove;
    }
}

int GetFreeSpaceId(ax_display *Display, int ScreenID) {
    int totalSpaces = AXLibDisplaySpacesCount(Display);
    for (int i = 1; i <= totalSpaces; ++i) {
        space_identifier SI = {ScreenID, i};
        if (!SpaceIdentifierToVspace.count(SI)) {
            return i;
        }
    }
    return NOSPACEAVAILABLE;
}


void MoveVspaceToDisplay(ax_display *Display, std::string VspaceID) {

    int SpaceID = AXLibDesktopIDFromCGSSpaceID(Display, Display->Space->ID);
    int ScreenID = Display->ArrangementID;
    space_identifier ActiveSpaceID = {ScreenID, SpaceID};


    DEBUG("ScreenID: " + std::to_string(ScreenID) + ", SpaceID: " + std::to_string(SpaceID));
    if (!VspaceToSpaceIdentifier.count(VspaceID)) {
         DEBUG("Virtual space " + VspaceID + " has not been defined");
         return;
    }

    space_identifier PreviousSpaceID = VspaceToSpaceIdentifier[VspaceID];
    DEBUG("Moving from " + std::to_string(PreviousSpaceID.ScreenID) + ":" + std::to_string(PreviousSpaceID.SpaceID)
          + " to " + std::to_string(ActiveSpaceID.ScreenID) + ":" + std::to_string(ActiveSpaceID.SpaceID));
    switch(getMoveType(PreviousSpaceID, ActiveSpaceID)) {
        case VspaceNoMove:
        DEBUG("No move");
          break;
        case VspaceNewSpace:
        DEBUG("new space");

          if (SpaceIdentifierToVspace.count(ActiveSpaceID)) {
              // Current space is already assigned, lets find a new one.
              int NewSpaceId = GetFreeSpaceId(Display, ScreenID);
              if (NewSpaceId == NOSPACEAVAILABLE) {
                  DEBUG("ERROR could not find an available space");
                  return;
              }
            ActivateSpaceWithoutTransition(std::to_string(NewSpaceId));
              ActiveSpaceID.SpaceID = NewSpaceId;
          }
          // This space is free, lets just use it.
          break;
        case VspaceSameScreenMove:
          DEBUG("same screen");
          ActivateSpaceWithoutTransition(std::to_string(PreviousSpaceID.SpaceID));
          ActiveSpaceID = PreviousSpaceID;

          break;
        case VspaceDifferentScreenMove:
            DEBUG("different screen");

          if (SpaceIdentifierToVspace.count(ActiveSpaceID)) {
              // Current space is already assigned, lets find a new one.
              int NewSpaceId = GetFreeSpaceId(Display, ScreenID);
              if (NewSpaceId == NOSPACEAVAILABLE) {
                  DEBUG("ERROR could not find an available space");
                  return;
              }
            ActivateSpaceWithoutTransition(std::to_string(NewSpaceId));
              ActiveSpaceID.SpaceID = NewSpaceId;
          }
          // LoadSpaceSettings

          // Move all windows from previous space to new space on the new monitor
          ax_display *OtherDisplay = AXLibArrangementDisplay(PreviousSpaceID.ScreenID);
          int OtherSpace = AXLibDesktopIDFromCGSSpaceID(OtherDisplay, OtherDisplay->Space->ID);

          // Temporarily move to previous space if it isn't already the current one.
          bool MustChangeSourceSpace = OtherSpace != PreviousSpaceID.SpaceID;
           if (MustChangeSourceSpace) {
               FocusDisplay(OtherDisplay);
               LeftClickToFocus();
               ActivateSpaceWithoutTransition(std::to_string(PreviousSpaceID.SpaceID));
           }

           std::vector<ax_window *> Windows = AXLibGetAllVisibleWindows();

           for (auto &Window : Windows) {
               ax_display *WindowDisplay = AXLibWindowDisplay(Window);
               DEBUG("Window!");
               if (WindowDisplay == OtherDisplay) {
                   DEBUG("Moving window");
                   MoveWindowToDisplay(Window, Display);
               }
           }

           // if (MustChangeSourceSpace) {
           //  // Reset the original space in the source display and give back focus to the main display.
           //      ActivateSpaceWithoutTransition(std::to_string(OtherSpace));
           //      FocusDisplay(Display);
           //      LeftClickToFocus();
           //
           // }


          break;

          // Reset space settings for old display.
    }
    // Remove space mapping if any.
    SpaceIdentifierToVspace.erase(PreviousSpaceID);

    VspaceToSpaceIdentifier[VspaceID] = ActiveSpaceID;
    SpaceIdentifierToVspace[ActiveSpaceID] = VspaceID;

    // void MoveWindowToDisplay(ax_window *Window, ax_display *NewDisplay)
    // void ActivateSpaceWithoutTransition(std::string SpaceID)
}

void MoveWindowToVspace(ax_display *Display, ax_window *Window, std::string VspaceID) {
    DEBUG("Moving window");
    if (!Window) {
      DEBUG("No active window found");
      return;
    }
    if (!VspaceToSpaceIdentifier.count(VspaceID)) {
         DEBUG("Virtual space " + VspaceID + " has not been defined");
         return;
    }

    int SpaceID = AXLibDesktopIDFromCGSSpaceID(Display, Display->Space->ID);
    int ScreenID = Display->ArrangementID;
    space_identifier CurrentSpaceID = {ScreenID, SpaceID};

    space_identifier NewSpaceID = VspaceToSpaceIdentifier[VspaceID];

    DEBUG("Moving from " + std::to_string(CurrentSpaceID.ScreenID) + ":" + std::to_string(CurrentSpaceID.SpaceID)
          + " to " + std::to_string(NewSpaceID.ScreenID) + ":" + std::to_string(NewSpaceID.SpaceID));
    switch (getMoveType(CurrentSpaceID, NewSpaceID)) {
        case VspaceNoMove:
          DEBUG("No move");
          return;
        case VspaceNewSpace:
          DEBUG("new space");
          int NewSpace;
          // Initialise new space on the same screen and move window there.
          if (SpaceIdentifierToVspace.count(CurrentSpaceID)) {
              // Current space is already assigned, lets find a new one.
              NewSpace = GetFreeSpaceId(Display, ScreenID);
              if (NewSpace == NOSPACEAVAILABLE) {
                  DEBUG("ERROR could not find an available space");
                  return;
              }
          } else {
            NewSpace = CurrentSpaceID.SpaceID;
          }

          NewSpaceID.ScreenID = CurrentSpaceID.ScreenID;
          NewSpaceID.SpaceID = NewSpace;

          // Initialise vspace.
          VspaceToSpaceIdentifier[VspaceID] = NewSpaceID;
          SpaceIdentifierToVspace[NewSpaceID] = VspaceID;

          // Move window to space.
          MoveFocusedWindowToSpace(std::to_string(NewSpaceID.SpaceID));
          break;
        case VspaceSameScreenMove:
          DEBUG("same screen");
          MoveFocusedWindowToSpace(std::to_string(NewSpaceID.SpaceID));
          return;
        case VspaceDifferentScreenMove:
           DEBUG("different screen");

          // Move all windows from current space to new space on the new monitor
          ax_display *OtherDisplay = AXLibArrangementDisplay(NewSpaceID.ScreenID);
          int OtherSpace = AXLibDesktopIDFromCGSSpaceID(OtherDisplay, OtherDisplay->Space->ID);

          // Temporarily move to previous space if it isn't already the current one.
          bool MustChangeSourceSpace = OtherSpace != NewSpaceID.SpaceID;

           if (MustChangeSourceSpace) {
               FocusDisplay(OtherDisplay);
               LeftClickToFocus();
               ActivateSpaceWithoutTransition(std::to_string(NewSpaceID.SpaceID));
           }

           MoveWindowToDisplay(Window, OtherDisplay);


           // if (MustChangeSourceSpace) {
           //  // Reset the other space in the target  display.
           // Return focus to previously active screen

          break;
    }

}


void AddVspace(std::string VspaceID) {
    DEBUG("Adding VSpace: " + VspaceID);
    if (!VspaceToSpaceIdentifier.count(VspaceID)) {
        space_identifier SpaceIdentifier = { UNASSIGNED, UNASSIGNED };
        VspaceToSpaceIdentifier[VspaceID] = SpaceIdentifier;
    }
}

void DeleteVspace(std::string VspaceID) {

}