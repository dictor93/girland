Introduce 
======
All names of the files, classes, folders, schematics, projects - all must use [CamelCase style](https://ru.wikipedia.org/wiki/CamelCase) and must have a sense and describes possibility of the code.

1 Folders, files structures
======
1.1 Tree of the repo
------
> 
    * Wiki 
    * Projects
        * Software
            * App
                * IarProject0
                * …
                * IarProjectN
                * QtProject0 
                * ….
                * QtProjectN
                * AndroidStudioProject0 
                * ….
                * AndroidStudioProjectN
            * Libs
                * Header 
                    * Drivers 
                        * Platform0 
                        * …
                        * PlatformN 
                    * Module0
                    * …  
                * Source 
                    * Drivers 
                        * Platform0 
                        * …
                        * PlatformN 
                    * Module0
                    * …  
            * Resource
            * Web 
        * Hardware
            * AltiumProject1
            * ….  
            * AltiumProjectN
    * Examples
    * Install 
>

1.2 A file of the bridge between different blatfor
======
>Note! If we use cross platform code or need to build the same code for different platform - we need to use this file ["Porting.h"](https://gitlab.com/droid-technologies/Information/tree/master/Template/Projects/Lips/Header/Porting.h)
 
The file must have some defines which will use in the different platform. 
The basic code is:

#inlude "stdint.h"
#define WAIT(ms)                     delay(ms)
#define ENABLE_INTERRUPT()           enable_interrupt()
#define DISABLE_INTERRUPT()          disable_interrupt()
#ifdef STM32F0
...
#else
...
#endif

1.3 A file of the typedefs 
======
> Note! If a IDE (Keil, Iar...) doesn't have a file with typedefs, we must use a file ["Typedefs.h"](https://gitlab.com/droid-technologies/Information/tree/master/Template/Projects/Lips/Header/Typedefs.h), it must be included only in the file ["Porting.h"](https://gitlab.com/droid-technologies/Information/tree/master/Template/Projects/Lips/Header/Porting.h) 

2 Code style
======
2.1 Name of the files
------
**Table 2.1.1**

C++| C| Python
--- | --- | ---
**TestFile.cpp** | **TestFile.c** | **TestFile.py** 
**TestFile.h** | **TestFile.h** |      **-**


2.2 Name of the clases, structures, unions
------
Must have the same name how files of the class.
>NOTE! If we have a few structures, unions or class + structures we should give a name for the main class/structure how name of the header file. For other need to add the next part of the name.

**Table 2.2.1 - Names of the objetcs for different Lanquage**

Lanquage|Class| Structure| Unionn
--- | --- | ---| --- 
C++|TestObject|TestObject::MyStruct|TestObject::MyUnion
C|TestObject - class change to structure|TestObjectMyStruct|TestObjectMyUnion
Python|TestObject|TestObject::MyStruct|TestObject::MyUnion

The amin class/structure/union|TestObject|TestObject | TestObject

2.3 Name of the functios
------
Each name of a function/method must start from small latter and must have start part of the file name.

**Table 2.3.1 - Names of the function for different Lanquage**

Lanquage|Function name original|Function
--- | --- | ---
C++|setData|TestObject::setData
C|setData|TestObject_setData
Python|setData|TestObject::setData

2.3 Name of the variables
------
Lanquage|Local(inside function/method)|Global(static)|inside class/structure
--- | --- | ---| ---
C++|l_testVariable|s_testVariable|m_testVariable
C|l_testVariable|s_testVariable|m_testVariable
Python|l_testVariable|s_testVariable|m_testVariable

>Note! We don't use global, external variables at all, we use only static varibles
>Note! Don't need to tell type of the variable, now IDE show it.

2.4 Style of functions, structures, classes
------
Each name of a function/method must start from small latter and must have start part of the file name.

Lanquage|Function name original|Function
--- | --- | ---
C++|setData|TestObject::setData
C|setData|TestObject_setData
Python|setData|TestObject::setData

>Note! Braces should start after press enter in the first letter in the line.

2.5 Style of operators
------
It should be used for all operators: if, if else, do while, for, operator, switch case ...
>Note! Braces should start in the same line how  a operator  after one empty letter.

Example:
```javascript
if (...) {
    int32_t l_test = 0;
}

for (uint8_t l_counter = 0; l_counter < 3; l_counter++) {
    int32_t l_test = 0;
}

while (1) {
    int32_t l_temp = 0;
    ...
}

operator uint8_t() {
    ...
}

...
```
 
 2.5 Style of operators
------
It should be used for all operators: if, if else, do while, for, operator, switch case ...
>Note! Braces should start in the same line how  a operator  after one empty letter.

 
 
