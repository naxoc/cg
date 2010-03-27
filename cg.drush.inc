<?php

/**
 * Implementation of hook_drush_help().
 */
function cg_drush_help($section) {
  switch ($section) {
    case 'drush:cg-plugin':
      return dt('Creates a plugin file with stub code in the module dir you are in when issuing the command. Currently supports content_types, styles and tasks.');
    }
}

/**
 * Implementation of hook_drush_command().
 */
function cg_drush_command() {
  $items['cg-plugin'] = array(
    // We're skipping the customary drush_ prefix, as it increases the chance
    // of colliding with a hook.
    'callback' => 'cg_create_ctools_plugin',
    'description' => 'Create ctools plugins.',
    'arguments' => array(
      'type' => 'The type of ctools plugin. Mandatory.',
      'name' => 'The name of the ctools plugin to be created. Mandatory.',
    ),
    'examples' => array(
      'cg create layout my_layout' => 'Creates a layout called my_layout in the module folder.',
    ),
  );
  return $items;
}

function cg_create_ctools_plugin() {
  $module = cg_get_module();
  if (!$module) {
    return FALSE;
  }
  $arguments = func_get_args();
  if (empty($arguments[0])) {
    return drush_set_error(dt('No plugin type specified.'));
  }
  if (empty($arguments[1])) {
    return drush_set_error(dt('No name specified.'));
  }

  $plugin_name = $arguments[1];
  $plugin_type = $arguments[0];

  if (!in_array($plugin_type, array('content_type', 'style', 'task'))) {
    return drush_set_error(dt('I do not know the plugin type you specified.'));
  }

  // Get the naming logic.
  $prefix = $module .'_'. $plugin_name;

  require_once 'plugins/'. $plugin_type .'.inc';
  $file_code = call_user_func('cg_generate_plugin_'. $plugin_type, $plugin_name, $prefix);
  $filename = cg_create_plugin_file_name($plugin_type, $plugin_name, TRUE);

  cg_write($filename, $file_code);
}

function cg_create_plugin_file_name($plugin_type, $plugin_name) {
  // Change to the current module dir.
  chdir(drush_cwd());
  if (!file_exists('plugins')) {
    drush_op('mkdir', 'plugins');
  }
  // The file folders for ctools plugins have the plugin type pluralized.
  $dir = 'plugins/'. $plugin_type .'s';
  if (!file_exists($dir)) {
    drush_op('mkdir', $dir); 
  }
  if (in_array($plugin_type, array('content_type'))) {
    // Content types go in their own folder in the plugins dir, so make that dir.
    $dir .= '/'. $plugin_name;
    if (!file_exists($dir)) {
     drush_op('mkdir', $dir);
    }
  }
  return $dir .'/'. $plugin_name .'.inc';
}

function cg_get_module() {
  // Find a module.
  // Apparently drush thinks we're in the modules directory, while we're in
  // fact in the site root. Odd.
  $old = getcwd();
  chdir(drush_cwd());
  $modules = glob('*.module');
  chdir($old);
  $candidates = array();
  foreach ($modules as $name) {
    $candidates[] = basename($name, '.module');
  }

  if (sizeof($candidates) == 1) {
    return $candidates[0];
  } else {
    return drush_set_error(dt('No module.'));
  }
}

function cg_write($filename, $content) {
  file_put_contents($filename . '.new', $content);
  if (file_exists($filename)) {
    drush_op('rename', $filename, $filename . '.em~');
  }
  drush_op('rename', $filename . '.new', $filename);
}
