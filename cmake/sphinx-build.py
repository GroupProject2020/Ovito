# -*- coding: utf-8 -*-
import sys

if __name__ == '__main__':

    try:
        from sphinx.cmd.build import main
    except ImportError: # Fallback for older Sphinx versions:
        from sphinx import main

    sys.exit(main())