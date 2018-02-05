///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <mach-o/dyld.h>

/**
 * Small helper program that executes the Ovito.app/Contents/MacOS/ovitos executable,
 * which is located in a nested app bundle under macOS. 
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
