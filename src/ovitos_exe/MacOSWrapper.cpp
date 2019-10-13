////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <mach-o/dyld.h>

/**
 * Small helper program that executes the Ovito.app/Contents/MacOS/ovitos executable,
 * which is located in a nested app bundle on the macOS platform.
 */
int main(int argc, char* argv[], char *envp[])
{
	// Get path to this executable.
	uint32_t size = 0;
	_NSGetExecutablePath(nullptr, &size);
	char* path = new char[size + 128];
	_NSGetExecutablePath(path, &size);

	// Append path to the executable in the nested bundle.
	char* slash = strrchr(path, '/');
	if(slash == nullptr) slash = path;
	else slash++;
	strcpy(slash, "Ovito.app/Contents/MacOS/ovitos");

	// Execute process.
	execve(path, argv, envp);
	delete[] path;
	printf("executing subprocess: %s\n", path);
	perror("execve failed");
	return 1;
}
