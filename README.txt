Gittyup is a set of drush commands to update a Drupal install's git repo.

Update a git repository by performing a git pull --rebase. It is assumed that
the root directory of the git repository is the root of the Drupal install. If
this is not the case, it is recommended to explicitly declare your repository's
root in the site-alias for the site:

$aliases['foo']['git-root'] = '/bar/baz';

You may also specify it inline with the git-root option:

drush gittyup --git-root='/bar/baz'