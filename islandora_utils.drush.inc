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
    'islandora-utils-index-list' => array(
      'drupal dependencies' => array(
        'islandora',
        'islandora_basic_collection',
      ),
      'options' => array(
        'list' => array(
          'description' => dt('list of comma-separated PIDs to reindex.'),
          'required' => TRUE,
        ),
      ),
    ),
    'islandora-utils-get-pids-for-collection' => array(
      'drupal dependencies' => array(
        'islandora',
        'islandora_basic_collection',
      ),
      'options' => array(
        'collection' => array(
          'description' => dt('collection pid'),
          'required' => TRUE,
          ),
        'outfile' => array(
          'description' => dt('File to write list.'),
          'required'    => FALSE,
          )
        ),
    )
  );
}


function drush_islandora_utils_get_pids_for_collection(){
    require_once('includes/gsearch.inc');
    $pid = drush_get_option('collection');
    $pids = get_collection_members($pid);
    $outString = csv_format_pid_list($pids);
    if(drush_get_option('outfile')){
        print('writing file'.drush_get_option('outfile'));
        file_put_contents(drush_get_option('outfile'), $outString);
    }else{
//        drush_log($outString, 'success');
        print "\n".$outString."\n\n";
    }
}

/**
 * Drush command for islandora-basic-collection-generate-thumbs-from-children.
 */
function drush_islandora_utils_reindex_collection() {
    require_once('includes/gsearch.inc');
    $pid = drush_get_option('collection');
    $pids = get_collection_members($pid);
    process_pid_list($pids);
}

function drush_islandora_utils_index_list(){
    $pids = explode(',', drush_get_option('list'));
    process_pid_list($pids);
}

function process_pid_list($list){
    require_once('includes/gsearch.inc');

    $last = 0;
    $failures = array();

    foreach($list as $pid){
        timer_start('gsearch');

        if(!speak_to_gsearch($pid, 'index')){
            $failures[] = $pid;
        }

        $timer   = timer_stop('gsearch');
        $elapsed = $timer['time'] - $last;
        drush_log("Reindexed $pid.       ". format_times($timer, $elapsed), 'success');

        $last    = $timer['time'];
    }
    if(!empty($failures)){
        drush_log("The followingt pids failed reindexing: ".  csv_format_pid_list($failures), 'warning');
    }
}

function csv_format_pid_list($pids){
    if(!is_array($pids)){
        throw new Exception('Input param is not an array.');
    }
    return implode(',',$pids);
}
