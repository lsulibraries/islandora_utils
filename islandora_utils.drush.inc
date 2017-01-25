<?php

/**
 * @file
 * Drush commands.
 */

/**
 * Implements hook_drush_command().
 */
function islandora_utils_drush_command() {
  return array(
    'islandora-utils-reindex-collection' => array(
      'description' => dt('Update index via gsearch.'),
      'examples' => array('drush islandora-reindex-collection-reindex-collection --collection=my:collection'),
      'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
      'drupal dependencies' => array(
        'islandora',
        'islandora_basic_collection',
      ),
      'options' => array(
        'collection' => array(
          'description' => dt('PID of the collection to reindex.'),
          'required' => TRUE,
        ),
      ),
    ),
  );
}

/**
 * Drush command for islandora-basic-collection-generate-thumbs-from-children.
 */
function drush_islandora_utils_reindex_collection() {
    require_once('includes/gsearch.inc');
    $pid = drush_get_option('collection');
    $members = get_collection_members($pid);
    $last = 0;
    $failures = array();

    foreach($members as $member){
        timer_start('gsearch');

        if(!speak_to_gsearch($member, 'index')){
            $failures[] = $member;
        }

        $timer   = timer_stop('gsearch');
        $elapsed = $timer['time'] - $last;
        drush_log("Reindexed $member.       ". format_times($timer, $elapsed), 'success');

        $last    = $timer['time'];
    }
    if(!empty($failures)){
        drush_log("The followingt pids failed reindexing: ".implode(',',$failures), 'warning');
    }
}
