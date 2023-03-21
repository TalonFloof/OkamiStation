#pragma once

void SCSIInit();
void SCSIAttachDrive(const char* path);
void SCSICloseDrives();
void SCSITick();