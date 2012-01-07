PROJET 103
=================

Projet test de monitoring (CPU, IP...) et de centralisation serveur.

* \#Num: #1

Pour récupérer un version du projet
-----------------------------------

```
$ git clone git@github.com:azman0101/xproj103.git
```
Usage
-----

```bash
$ cd xproj103
$ make clean
$ make
$ ./src/xproj103 
```



Versioning
----------



Releases will be numbered with the follow format:

`<major>.<minor>.<patch>`

And constructed with the following guidelines:

* Breaking backwards compatibility bumps the major
* New additions without breaking backwards compatibility bumps the minor
* Bug fixes and misc changes bump the patch



Bug tracker
-----------

Have a bug? Please create an issue here on GitHub!


Twitter account
---------------

Mailing list
------------



Developers
----------


+ **build** - `make build`
This will run the less compiler on the bootstrap lib and generate a bootstrap.css and bootstrap.min.css file.
The lessc compiler is required for this command to run.

+ **watch** - `make watch`
This is a convenience method for watching your less files and automatically building them whenever you save.
Watchr is required for this command to run.


Authors
-------

**Julien BOULANGER**

+ http://twitter.com/azman0101
+ http://github.com/azman0101


License
---------------------

