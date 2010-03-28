CG is an abbreveiation for Code Generate.

So far it supports three kinds of ctools plugins:
* content_type
* style
* task

To use it, change dirs to the module you would like to be responsible for the 
plugin. Then issue the command 'drush cg-plugin content_type somename'.

You can supply the option -w to have cg write an implementation of the
hooks needed for ctools to find the plugin.