# php_yugioh
Yu-Gi-Oh! PHP extension.

## Cloning
```
$ git clone https://github.com/jonas-vanen/php_yugioh.git --recursive
```

## Building
### Linux
##### Requirements
* PHP version `7.*`
* SQLite developer package (sqlite-devel)
##### Compiling
```
$ phpize
$ ./configure
$ make
```

### Windows
##### Requirements
* PHP version `7.*` (binaries and source)
* SQLite amalgamation source code from https://sqlite.org/
##### Compiling
Open the Visual Studio Solution file **`yugioh.sln`**.  
Add **`sqlite3.c`** to the source files and set the following configuration properties:  

* `C/C++ > General > Additional Include Directories`:  
  - The directory containing **`sqlite3.h`**  
  - *`php-source`*, *`php-source\main`*, *`php-source\Zend`*, *`php-source\TSRM`* and *`php-source\win32`  

* `Linker > General > Additional Library Directories`:  
  - **`php\dev`** containing **`php7ts.lib`**  
  
* `Linker > Input > Additional Dependencies`:  
  - **`php7ts.lib`**
  
Now set your configuration and platform and build the DLL.

## Overview
```php
class yugioh\card
{
    public integer $original_id;
    public array $ids;
    public string $name;
    public string $desc;
    public integer $ot;
    public integer $setcode;
    public integer $type;
    public integer $atk;
    public integer $def;
    public integer $level;
    public integer $race;
    public integer $attribute;
    public integer $category;
}

class yugioh\replay
{
    public integer $life_points;
    public integer $hand_count;
    public integer $draw_count;
    public array $players;

    public static function from_file(string $file): yugioh\replay;
    public function read_file(string $file): void;
    public function decode(string $data): void;
}

class yugioh
{
    public array $dbs;

    public function db(string $language, string $path): void;
    public function dbs(array $paths): void;
    public function db_remove(string $language): void;
    public function match(string $name, string $inlang, number $count = 1): array;
    public function search(string $any, string $inlang, string $lang): yugioh\card;
}
```


#
This repository is licensed under the GNU General Public License.  
©2017 Jonas Vanen
