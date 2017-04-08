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
Build SQLite3 as a static library, then open the Visual Studio Solution file **`yugioh.sln`**.  
Add **`sqlite3.c`** to the source files and set the following configuration properties:  

* `C/C++ > General > Additional Include Directories`:  
  - The directory containing **`sqlite3.h`**
  - *`php-source`*, *`php-source\main`*, *`php-source\Zend`*, *`php-source\TSRM`* and *`php-source\win32`  

* `Linker > General > Additional Library Directories`:
  - The directory containing the SQLite3 static library
  - **`php\dev`** containing **`php7ts.lib`**
  
Now set your configuration and platform and build the DLL.


#
This repository is licensed under the GNU General Public License.  
©2017 Jonas Vanen
