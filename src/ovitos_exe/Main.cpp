////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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

#include <ovito/gui/GUI.h>
#include <ovito/gui/app/GuiApplication.h>

/**
 * This is the main entry point for the "ovitos" script interpreter program.
 *
 * This function translates command line arguments to the format expected by the
 * OVITO core application. Note that most of the application logic is found in the
 * Core module of OVITO. Script execution is performed by the PyScript plugin module.
 */
int main(int argc, char** argv)
{
	std::vector<const char*> newargv;
	newargv.push_back(*argv++);
	argc--;

	const char* loadFile = nullptr;
	bool graphicalMode = false;
	bool execMode = false;
	while(argc > 0) {
		if(strcmp(*argv, "-o") == 0) {
			if(argc >= 2)
				loadFile = argv[1];
			argv += 2;
			argc -= 2;
		}
		else if(strcmp(*argv, "-m") == 0) {
			if(argc >= 2) {
				newargv.push_back("--exec");
				static std::string moduleCommand("import runpy; runpy.run_module('");
				moduleCommand += argv[1];
				moduleCommand += "', run_name='__main__');";
				newargv.push_back(moduleCommand.c_str());
			}
			argv += 2;
			argc -= 2;
			execMode = true;
			break;
		}
		else if(strcmp(*argv, "-u") == 0) {
			argc--;
			argv++;
			// Silently ignore the -u command line option.
			// We only accept it for the sake of compatibility with CPython.
		}
		else if(strcmp(*argv, "-c") == 0) {
			if(argc >= 2) {
				newargv.push_back("--exec");
				newargv.push_back(argv[1]);
			}
			argv += 2;
			argc -= 2;
			execMode = true;
			break;
		}
		else if(strcmp(*argv, "-nt") == 0 || strcmp(*argv, "--nthreads") == 0) {
			if(argc >= 2) {
				newargv.push_back("--nthreads");
				newargv.push_back(argv[1]);
			}
			argv += 2;
			argc -= 2;
			break;
		}
		else if(strcmp(*argv, "-h") == 0 || strcmp(*argv, "--help") == 0) {
			std::cout << "OVITO Script Interpreter" << std::endl << std::endl;
			std::cout << "Usage: ovitos [-g|--gui] [-v|--version] [-nt|--nthreads <NumThreads>] [-o FILE] [-c command | -m module-name | script-file] [arguments]" << std::endl;
			return 0;
		}
		else if(strcmp(*argv, "-v") == 0 || strcmp(*argv, "--version") == 0) {
			argc--;
			newargv.push_back(*argv++);
		}
		else if(strcmp(*argv, "-g") == 0 || strcmp(*argv, "--gui") == 0) {
			argc--;
			argv++;
			graphicalMode = true;
		}
		else break;
	}
	if(!graphicalMode)
		newargv.insert(newargv.begin() + 1, "--nogui");

	if(!execMode) {
		if(argc >= 1) {
			// Parse script name and any subsequent arguments.
			newargv.push_back("--script");
			newargv.push_back(*argv++);
			argc--;
		}
		else {
			if(graphicalMode) {
				std::cerr << "ERROR: Cannot run interactive Python interpreter in graphical mode. Only non-interactive script execution is allowed." << std::endl;
				return 1;
			}

			// If no script file has been specified, activate interactive interpreter mode.
			newargv.push_back("--exec");
			newargv.push_back(
					"import sys\n"
					"try:\n"
					"    import IPython\n"
					"    print(\"This is OVITO\'s interactive IPython interpreter. Use quit() or Ctrl-D to exit.\")\n"
					"    IPython.start_ipython(['--nosep','--no-confirm-exit','--no-banner','--profile=ovito','-c','import ovito','-i'])\n"
					"    sys.exit()\n"
					"except ImportError:\n"
					"    pass\n"
					"import ovito\n"
					"import code\n"
					"code.interact(banner=\"This is OVITO\'s interactive Python interpreter. "
#if WIN32
					"Use quit() or Ctrl-Z to exit.\")\n"
#else
					"Use quit() or Ctrl-D to exit.\")\n"
#endif
				);
		}
	}

	// Escape script arguments with --scriptarg option.
	while(argc > 0) {
		newargv.push_back("--scriptarg");
		newargv.push_back(*argv++);
		argc--;
	}

	// The OVITO file to be loaded must come last in the parameter list passed to OVITO.
	if(loadFile) newargv.push_back(loadFile);

	// Initialize the application.
	int newargc = (int)newargv.size();
	Ovito::GuiApplication app;
	if(!app.initialize(newargc, const_cast<char**>(newargv.data())))
		return 1;

	// Enter event loop.
	int result = app.runApplication();

	// Shut down application.
	app.shutdown();

	return result;
}
