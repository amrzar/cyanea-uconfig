# Language

There are only three constructs supported by cyanea-uconfig: *Menu*, *Configuration Option*, and *Multiple Choices*

## Menu [*menu* ... *endmenu*]

Create a menu with "*menu_id*"  name which contains any of the supported constructs. "*menu_id*" appears in GUI.

```
menu "menu_id"
	[depends expression]
	[...]
endmenu
```
## Configuration Option [*config* ...]

Create a configuration option with "*config_id*" name. The name is optional and can be empty if the configuration option should not appear in the GUI.

```
config ["config_id"]
	SYMBOL
	TYPE initial_value
	[select select_list]
	[depends expression]
	[help help_info]
```

### **SYMBOL**

A *SYMBOL* is used in output file as configuration variable name. *SYMBOL*  can be uppercase alphanumeric characters and underscore. Some valid names are *CONFIG_ARCH* or *CONFIG_DEBUG_BUILD*. The initial character cannot be a number.

### **Variable type**

With "*TYPE* initial_value",  user can define type of *SYMBOL* and initialise its default value. *TYPE* can be **BOOL** for boolean, **INTEGER** for integer and **STRING** for string variables. For boolean variable, *initial_value*  can be **true** or **false**, for string variable **"..."** is necessary.

Example: *CONFIG_MULTIBOOT_GRAPHICS_HEIGHT* is an integer symbol.

```
config "Screen height"
	CONFIG_MULTIBOOT_GRAPHICS_HEIGHT
	INTEGER 0
	help
		"Specify number of the columns.
		 This is specified in pixels in EGA text mode,
		 and in characters in linear mode. 
		 (zero indicates that the OS has no preference)"
```

### **Select**

For configuration option with boolean *SYMBOL*, user can select other boolean symbols. If *SYMBOL* is set to *True*, boolean symbols specified in the *select_list* will be set to *True*. Similarly, if *SYMBOL* is set to *False*, boolean symbols specified in the *select_list* will be set to *False*.

cyanea-uconfig updates symbols in *select_list* only if it is safe to do so. For instance, if two configuration options select same boolean symbol, it only sets that boolean symbol to *False* if both configuration options are *False*.

**select** is optional.

### **Dependancy**

With "**depends** expression", user can define dependency between different constructs. The construct is enabled only If the expression evaluates to *True*.

The expresstion can be constructed using **==**, **!=**, **NOT**, **&&**, and **||**'. Equity comparison (**==** and **!=**) can be done between booleans, integers, and strings. Logical operations (**NOT**, **&&**, and **||**) are possible between two expressions. A boolean symbol is considered as an expression. The precedence of logical operation is from left-to-right, but user can use parentheses to change the precedence.

Example: menu "x86 Processor" is enable only if *CONFIG_ARCH* is "x86".

```
menu "x86 Processor"
	depends CONFIG_ARCH == "x86"
	[...]
endmenu
```

**depends** is optional.

### **Help**

With "**help** help_info", user can write documentation for different constructs. Fot *help_info*, **"..."** is necessary. 

**help** is optional.

## Multiple Choices [*config* ...]

Create a multiple choice with “choice_id“ name which contains list of options. “choice_id“ appears in GUI. 

```
choice "choice_id"
	SYMBOL
	option OP1 [option OP]
	[depends expression]
	[help help_info]
```

With "**option** OP", user can list possible options. *OP* can be string or integer. For string *OP*, **"..."** is necessary. **[default]** set the initial selected option.

Example: *CONFIG_X86_MICROARCHITECTURE* is string symbol with multiple options.

```
choice "x86 microarchitecture"
	CONFIG_X86_MICROARCHITECTURE
	option "native" [default]
	option "nehalem"
	option "westmere"
	option "sandybridge"
	option "ivybridge"
	option "haswell"
	option "broadwell"
	option "skylake"
```

## Include configuration files

With "**.include** path", user can include other files in a configuration file. The path can be absolute or relative to the root of the configuration files.
