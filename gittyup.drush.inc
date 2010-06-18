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
    'description' => "Update a git repository by performing a git pull --rebase. It is assumed that the root directory of the git repository is the root of the Drupal install. If this is not the case, it is recommended to explicitly declare your repository's root in the site-alias for the site ($\aliases['foo']['git-root'] = '/bar/baz';). You may also specify it inline with the git-root option (drush gittyup --git-root='/bar/baz')",
    'callback arguments' => array(array(), FALSE),
    'options' => array(
      '--git-root' => 'The root directory of the git repository from which to perform the update. Defaults to the root of the Drupal install.',
      '--git-tag' => 'Specific git tag to update to.',
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
    $tag = drush_get_option('git-tag', '');

    // Populate an array of commands to be executed, so we can implode them at the 
    // end with '; '. @see gittyup_shell_exec().
    $commands = array();
    // Our initial command just cd's into the git root. We separate it out as the 
    // first thing we need to do is verify that we actually have access to that 
    // directory.
    $cd = "cd $git_root";
    $commands[] = $cd;

    // Make sure we can get to the directory we need.
    if (drush_shell_exec($cd) || drush_get_context('DRUSH_SIMULATE')) {
      // Let's check to see if there's a tag first
      if ($tag) {
        $commands[] = "git fetch $origin"; 
        $commands[] = "git checkout $tag ./";
      }
      // If not, we're just going to do a git pull
      else {
        // Use an array to store options, and implode them at the end. Currently we only
        // have one option, but hey, there's room for growth!
        $options = array();
        // Check if we're rebasing.
        if (!drush_get_option('no-rebase', FALSE)) {
          $options[] = '--rebase';
        }
        // Now join all the options with a space.
        $options = implode(' ', $options);
        $options .= ' ';

        // Build the command.
        $commands[] = "git pull $options$origin $branch";
      }

      // Now we update the submodules.
      if (!drush_get_option('no-submodules', FALSE)) {
        $commands[] = "git submodule update --init";
      }

      gittyup_shell_exec($commands);
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
 *
 * @param $commands
 *   An array of string commands to execute.
 * @param $args
 *   An array of placeholder arguments for the commands.
 */
function gittyup_shell_exec($commands = array(), $args = array()) {
  $command = implode('; ', $commands);
  // Pop the command at the beginning of $args.
  $args = array_merge((array) $command, $args);

  $success = call_user_func_array('drush_shell_exec', $args);
  $output = implode("\n", (array) drush_shell_exec_output());

  if (!$success && !drush_get_context('DRUSH_SIMULATE')) {
    if ($output) {
      gittyup_check_error($output);
    }
    else {
      drush_set_error(dt('There was an error executing the following command: %command.', array('%command' => $command)));
    }
  }
  else if ($output) {
    if (!gittyup_check_error($output)) {
      drush_log($output, 'success');
    }
  }
}

/**
 * Checks for various git errors via regex on output.
 */
function gittyup_check_error($output) {
  $error = FALSE;

  if (preg_match('(needs update)', $output)) {
    drush_log($output, 'warning');
    drush_log(dt('You may have uncommitted changes to the repository. Try committing all changes and re-running the command.'), 'warning');
    $error = TRUE;
  }

  return $error;
}