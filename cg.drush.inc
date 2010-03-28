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
    'options' => array(
      '-i' => 'Write implementations of hook_ctools_plugin_dierctory() and hook_ctools_plugin_api() to the module file.',
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

  if (!empty($arguments[2])) {
    cg_write_include_in_module($module, $plugin_type, $plugin_name);
  }
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

function cg_write_include_in_module($module, $plugin_type, $plugin_name) {

  if (function_exists($module .'_ctools_plugin_directory')) {
    $func = new ReflectionFunction($module .'_ctools_plugin_directory');
    $filename = $func->getFileName();
    $code = file($filename);

    $start = $func->getStartLine();
    $end = $func->getEndLine() - 1;
    for ($i = $start; $i < $end; $i++) {
      if (preg_match('/\$plugin \=\= \'([a-zA-Z_]+)\'/', $code[$i], $matches)) {
        if ($matches[1] == $plugin_type. 's') {
          break; // We already have support for the type in the code.
        }
        $code[$i] = str_replace($matches[0], $matches[0] . ' || $plugin == \''. $plugin_type .'s\'', $code[$i]);
        file_put_contents($filename, $code);
        break;
      }
    }
  }
  else {
    $filename = drush_cwd() .'/'. $module .'.module';
    $code = file($filename);
    if ('?>' == trim(end($code))) {
      array_pop($code);
    }
    $code[] = "\n/**\n";
    $code[] = " * Implementation of hook_ctools_plugin_directory().\n";
    $code[] = " *\n";
    $code[] = " * Tells CTools (and thus Panels) where to look for plugin code.\n";
    $code[] = " */\n";
    $code[] = "function {$module}_ctools_plugin_directory(\$module, \$plugin) {\n";
    $code[] = " if ((\$module == 'ctools' || \$module == 'panels') && (\$plugin == '{$plugin_type}s')) {\n";
    $code[] = "   return 'plugins/' . \$plugin;\n";
    $code[] = " }\n";
    $code[] = "}\n";

    // Write the new function to the module.
    file_put_contents($filename, $code);
  }

  if (function_exists($module .'_ctools_plugin_api')) {
    $func = new ReflectionFunction($module .'_ctools_plugin_api');
    $filename = $func->getFileName();
    $code = file($filename);

    $exists = FALSE;
    $start = $func->getStartLine();
    $end = $func->getEndLine() - 2;
    for ($i = $start; $i < $end; $i++) {
      if (strpos($code[$i], '($module == \'page_manager\' && $api == \'pages_default\')')) {
        $exists = TRUE;
        break;
      }
    }
    if (!$exists) {
      $code[$end] .= "
  if (\$module == 'page_manager' && \$api == 'pages_default') {
    return array('version' => 1);
  }\n";
    file_put_contents($filename, $code);
    }
  }
  else {
    $filename = drush_cwd() .'/'. $module .'.module';
    $code = file($filename);
    if ('?' == trim(end($code))) {
      array_pop($code);
    }
    $code[] = "\n/**\n";
    $code[] = " * Implementation of ctools_plugin_api().\n";
    $code[] = " */\n";
    $code[] = "function {$module}_ctools_plugin_api() {\n";
    $code[] = " \$args = func_get_args();\n";
    $code[] = " \$module = array_shift(\$args);\n";
    $code[] = " \$api = array_shift(\$args);\n";
    $code[] = " if (\$module == 'page_manager' && \$api == 'pages_default') {\n";
    $code[] = "   return array('version' => 1);\n";
    $code[] = " }\n";
    $code[] = "}\n";
    // Write the new function to the module.
    file_put_contents($filename, $code);
  }
}

