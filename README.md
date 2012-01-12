PROJET 103
=================

Projet test de monitoring (CPU, IP...) et de centralisation serveur.

Pour récupérer un version du projet
-----------------------------------

```
$ git clone git@github.com:azman0101/xproj103.git
```
Usage
-----

```bash
$ cd xproj103
$ ./configure
$ make
$ ./src/xproj103 
```
En mode serveur:
./xproj103 [-a <ip de l'interface à écouter>] -p <port à écouter> -s

Si -a n'est pas utilisé, alors le serveur écoute toutes les interfaces (ipv4)
exemple: ./xproj103 -a 192.168.1.1 -p 1050 -s

En mode client:
./xproj103 -a <ip du serveur> -p <port d'écoute du serveur>

exemple: ./xproj103 -a 192.168.1.1 -p 1050 

Versioning
----------

Releases will be numbered with the follow format:

`<major>.<minor>`

* Bug fixes and misc changes bump the patch



Bug tracker
-----------

Un bug ? Ceci est un projet de cours sans autre interet que mon apprentissage :) !


Authors
-------

**Julien BOULANGER**

+ http://twitter.com/azman0101
+ http://github.com/azman0101


License
---------------------

