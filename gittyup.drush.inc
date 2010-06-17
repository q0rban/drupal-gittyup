<?php
// $Id$

/**
 * @file
 * Gittyup is a set of drush commands to update a Drupal install's git repo.
 */

/**
 * Implementation of hook_drush_help().
 */
function gittyup_drush_help($section) {
  switch ($section) {
    case 'drush:gittyup':
      return dt('Update a git repository via drush.');
  }
}

/**
 * Implementation of hook_drush_command().
 */
function gittyup_drush_command() {
  $items = array();

  $items['gittyup'] = array(
    'description' => "Update a git repository by performing a git pull --rebase. It is assumed that the root directory of the git repository is the root of the Drupal install. If this is not the case, it is recommended to explicitly declare your repository's root in the site-alias for the site ($aliases['foo']['git-root'] = '/bar/baz';). You may also specify it inline with the git-root option (drush gittyup --git-root='/bar/baz')"),
    'callback arguments' => array(array(), FALSE),
    'options' => array(
      '--git-root' => 'The root directory of the git repository from which to perform the update. Defaults to the root of the Drupal install.',
      '--git-branch' => 'Specify a git branch. Defaults to "master".',
      '--git-origin' => 'Specify a git origin. Defaults to "origin".',
      '--no-rebase' => 'Runs a git pull without the --rebase option. Useful for better separation of merged commits, but often rebasing is cleaner.',
      '--no-submodules' => 'Do not update submodules.',
    ),
    'aliases' => array('git-up'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );

  return $items;
}

/**
 * Command callback for drush gittyup.
 */
function drush_gittyup() {
  // Possibly auto detect with git rev-parse --show-cdup? @see http://j.mp/bGZE0r
  if ($git_root = drush_get_option('git-root', _drush_core_directory())) {
    $branch = drush_get_option('git-branch', 'master');
    $origin = drush_get_option('git-origin', 'origin');
  
    // Use an array to store options, and implode them at the end.
    $options = array();
    // Check if we're rebasing.
    if (!drush_get_option('no-rebase', FALSE)) {
      $options[] = '--rebase';
    }
    // Now join all the options with a space.
    $options = implode(' ', $options);
    $options .= ' ';
  
    // Build the command.
    $command = "cd $git_root; git pull $options$origin $branch";
  
    // Make sure we can get to the directory we need.
    if (drush_shell_exec("cd $git_root") || drush_get_context('DRUSH_SIMULATE')) {
      gittyup_shell_exec($command);
  
      // Now we update the submodules.
      if (!drush_get_option('no-submodules', FALSE)) {
        $command = "cd $git_root; git submodule update --init";
        gittyup_shell_exec($command);
      }
    }
    else {
      drush_set_error(dt('Cannot access %git-root', array('%git-root' => $git_root)));
    }
  }
  else {
    drush_set_error(dt('Could not find the git root. Please specify the git-root in the site alias, or declare it inline with --git-root.'));
  }
}

/**
 * Helper function to pass arguments through to drush_shell_exec().
 */
function gittyup_shell_exec($command) {
  $args = func_get_args();

  $success = call_user_func_array('drush_shell_exec', $args);
  $output = $output = implode("\n", (array) drush_shell_exec_output());

  if (!$success && !drush_get_context('DRUSH_SIMULATE')) {
    if ($output) {
      drush_set_error($output);
    }
    else {
      drush_set_error(dt('There was an error executing the following command: %command.', array('%command' => $command)));
    }
  }
  else if ($output) {
    drush_log($output, 'success');
  }
}