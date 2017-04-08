# php_yugioh
Yu-Gi-Oh! PHP extension.

## Cloning
```
$ git clone https://github.com/jonas-vanen/php_yugioh.git --recursive
```

## Linux
#### Requirements
* PHP version `7.*`
* SQLite developer package (sqlite-devel)
#### Compiling
```
$ phpize
$ ./configure
$ make
```

## Windows
#### Requirements
* PHP version `7.*` (binaries and source)
* SQLite amalgamation source code from https://sqlite.org/
#### Compiling
Build SQLite3 as a static library and call it **`sqlite3.lib`**, then open the Visual Studio Solution file **`yugioh.sln`**.  
Add **`sqlite3.c`** to the source files and update the additional include and library directories:  
Additional Include Directories:  
* Directory containing **`sqlite3.c`** and **`sqlite3.h`**.  
* Directories *`php-source`*, *`php-source\main`*, *`php-source\Zend`*, *`php-source\TSRM`* and *`php-source\win32`*.  
Additional Library Directories:  
* Directory containing the static library **`sqlite3.lib`**.  
* Directory *`php\dev`* containing **`php7ts.lib`**.  
Set your configuration and platform and build the DLL.


#
This repository is licensed under the GNU General Public License.  
©2017 Jonas Vanen
