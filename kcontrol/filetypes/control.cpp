#include "filetypesview.h"

extern "C"
{
	  TDE_EXPORT TDECModule *create_filetypes(TQWidget *parent, const char *)
          {
        return new FileTypesView(parent, "filetypes");
	  }

}

