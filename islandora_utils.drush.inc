<?php

/**
 * @file
 * Drush commands.
 */

/**
 * Implements hook_drush_command().
 */
function islandora_utils_drush_command() {
    $items = array();
  
    $items['islandora-utils-reindex-collection'] = array(
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
        'outfile' => array(
          'description' => dt('File to write list.'),
          'required'    => FALSE,
          )
      ),
    );
    $items['islandora-utils-index-list'] = array(
      'drupal dependencies' => array(
        'islandora',
        'islandora_basic_collection',
      ),
      'options' => array(
        'list' => array(
          'description' => dt('list of comma-separated PIDs to reindex.'),
          'required' => FALSE,
        ),
        'outfile' => array(
          'description' => dt('File to write list.'),
          'required'    => FALSE,
        ),
        'infile'  => array(
          'description' => dt('File containing a comma-separated list of pids'),
          'required'    => FALSE
        )
      ),
    );
    $items['islandora-utils-get-pids-for-collection'] = array(
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
    );
    $items['islandora-utils-get-objects-missing-dsid'] = array(
      'drupal dependencies' => array(
        'islandora',
        'islandora_basic_collection',
      ),
      'options' => array(
        'collection' => array(
          'description' => dt('collection pid'),
          'required' => TRUE,
          ),
        'dsid' => array(
          'description' => dt('The datastream to look for (case-sensitive).'),
          'required' => TRUE,
          ),
        'outfile' => array(
          'description' => dt('File to write list.'),
          'required'    => FALSE,
          ),
      ),
    );
  return $items;
}


function drush_islandora_utils_get_pids_for_collection(){
    require_once('includes/util.inc');
    $pid = drush_get_option('collection');
    $pids = get_collection_members($pid);
    output_result($pids);
}

/**
 * Drush command for islandora-basic-collection-generate-thumbs-from-children.
 */
function drush_islandora_utils_reindex_collection() {
    require_once('includes/util.inc');
    $pid = drush_get_option('collection');
    $pids = get_collection_members($pid);
    index_pids($pids);
}

function drush_islandora_utils_index_list(){
    require_once('includes/util.inc');
    $infile = drush_get_option('infile');
    $list   = drush_get_option('list');
    if($infile && $list){
        throw new Exception("'infile' and 'list' are mutually exclusive options.");
    }elseif (!$list) {
        $list = file_get_contents($infile);
    }
    $pids = explode(',', $list);
    index_pids($pids);
}

function drush_islandora_utils_get_objects_missing_dsid(){
    require_once('includes/util.inc');
    $coll  = drush_get_option('collection');
    $dsid  = drush_get_option('dsid');
    $pids  = get_collection_members($coll);
    $miss  = array();

    foreach($pids as $pid){
        $object = islandora_object_load($pid);

        if(!$object->getDatastream($dsid)){
            $miss[] = $pid;
            drush_log("Missing dsid $dsid for $pid", "warning");
            continue;
        }
        drush_log("$pid ok.", "success");
    }
    if(!empty($miss)){
        drush_log(sprintf("%d items missing the %s datastream.\n",count($miss), $dsid).  output_result($miss)."\n", 'warning');
    }else{
        drush_log("All objects in $coll have datastream $dsid", "success");
    }
}
