#ifndef AXLIB_SHAREDWORKSPACE_H
#define AXLIB_SHAREDWORKSPACE_H

#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <map>

#include "application.h"

void SharedWorkspaceSetApplicationsPointer(std::map<pid_t, ax_application> *Apps);
std::map<pid_t, std::string> SharedWorkspaceRunningApplications();
void SharedWorkspaceDidLaunchApplication(pid_t PID, std::string Name);
void SharedWorkspaceDidTerminateApplication(pid_t PID);

#endif
