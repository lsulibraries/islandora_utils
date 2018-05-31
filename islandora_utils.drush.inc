<?php
use Psr\Log\LogLevel;
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
    'examples' => array('drush islandora-reindex-collection-reindex-collection -u 1 active --collection=my:collection'),
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
        'description' => dt('File to write result list (relative to site root).'),
        'required' => FALSE,
      )
    ),
  );
  $items['islandora-utils-index-namespace'] = array(
    'drupal dependencies' => array(
      'islandora',
      'islandora_basic_collection',
    ),
    'options' => array(
      'namespace' => array(
        'description' => dt('Specify a namespace to reindex.'),
        'required' => FALSE,
      ),
      'outfile' => array(
        'description' => dt('File to write result list (relative to site root).'),
        'required' => FALSE,
      ),
    )
  );
  $items['islandora-utils-index-list'] = array(
    'drupal dependencies' => array(
      'islandora',
      'islandora_basic_collection',
    ),
    'options' => array(
      'list' => array(
        'description' => dt('String of comma-separated PIDs to reindex.'),
        'required' => FALSE,
      ),
      'outfile' => array(
        'description' => dt('File to write result list (relative to site root).'),
        'required' => FALSE,
      ),
      'infile' => array(
        'description' => dt('File containing a comma-separated string of pids'),
        'required' => FALSE,
      ),
    ),
  );
  $items['islandora-utils-get-cdm-pointers-for-collection'] = array(
    'drupal dependencies' => array(
      'islandora',
      'islandora_basic_collection',
    ),
    'options' => array(
      'collection' => array(
        'description' => dt('Collection PID.'),
        'required' => TRUE,
      ),
      'outfile' => array(
        'description' => dt('File to write result list (relative to site root).'),
        'required' => FALSE,
      ),
      'infile' => array(
        'description' => dt('File containing a comma-separated string of CDM pointers'),
        'required' => FALSE,
      ),
    ),
  );
  $items['islandora-utils-get-pids-for-collection'] = array(
    'drupal dependencies' => array(
      'islandora',
      'islandora_basic_collection',
    ),
    'options' => array(
      'collection' => array(
        'description' => dt('Collection PID.'),
        'required' => TRUE,
      ),
      'outfile' => array(
        'description' => dt('File to write result list (relative to site root).'),
        'required' => FALSE,
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
        'description' => dt('Collection PID.'),
        'required' => TRUE,
      ),
      'dsid' => array(
        'description' => dt('The datastream to look for (case-sensitive).'),
        'required' => TRUE,
      ),
      'outfile' => array(
        'description' => dt('File to write result list (relative to site root).'),
        'required' => FALSE,
      ),
    ),
  );
  $items['islandora-utils-get-objects-changed-dsid'] = array(
    'drupal dependencies' => array(
      'islandora',
      'islandora_basic_collection',
    ),
    'options' => array(
      'collection' => array(
        'description' => dt('Collection PID.'),
        'required' => TRUE,
      ),
      'dsid' => array(
        'description' => dt('The datastream to look for (case-sensitive).'),
        'required' => TRUE,
      ),
      'outfile' => array(
        'description' => dt('File to write result list (relative to site root).'),
        'required' => FALSE,
        ),
      ),
  );
  $items['islandora-utils-create-collection'] = array(
    'description' => dt('Create a collection container.'),
    'examples' => array('drush islandora-utils-create-collection --parent=islandora:root --namespace=my:collection'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
    'drupal dependencies' => array(
      'islandora',
      'islandora_basic_collection',
    ),
    'options' => array(
      'namespace' => array(
        'description' => dt('PID to assign to the collection.'),
        'required' => TRUE,
      ),
      'parent' => array(
        'description' => dt('Collection to which the new collection will belong; defaults to islandora:root.'),
        'required' => FALSE,
      ),
      'title' => array(
        'description' => dt('Collection title'),
        'required' => TRUE,
      ),
      'description' => array(
        dt("Description of the collection."),
        'required' => TRUE,
      )
    ),
  );
  $items['islandora-utils-set-state'] = array(
    'description' => dt('Set the fedora object state for a PID or for an entire namespace. OK to specify both options (pid/namespace), at least one of them is required.'),
    'examples' => array('drush islandora-utils-set-state-namespace --namespace=my --pid=my:pid'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
    'drupal dependencies' => array(
      'islandora',
    ),
    'options' => array(
      'namespace' => array(
        'description' => dt('namespace prefix to look for.'),
        'required' => FALSE,
      ),
      'pid' => array(
        'description' => dt('PID to set state for.'),
        'required' => FALSE,
      ),
      'logfile' => array(
        'description' => dt('Writable path for error logging.'),
        'required' => FALSE,
      ),
    ),
    'arguments' => array(
      'state' => 'inactive or active',
    ),
  );
  return $items;
}

function get_namespace_members($namespace) {
    $query = <<<EOL
    SELECT ?pid ?parent
    FROM <#ri>
    WHERE { {?pid <fedora-rels-ext:isMemberOfCollection> ?parent}
    UNION {?pid <fedora-rels-ext:isConstituentOf> ?parent}
    filter regex(str(?parent), "$namespace") } ORDER BY ?pid
EOL;

  // Use tuque to get a connection to Fedora and execute the SPARQL query.
  $tuque = islandora_get_tuque_connection();
  $repo = $tuque->repository;
  return $repo->ri->sparqlQuery($query);
}

function drush_islandora_utils_set_state($state) {
  $namespace = drush_get_option('namespace');
  $pid = drush_get_option('pid');
  if(!$namespace && !$pid) {
    drush_log("One of 'namespace' or 'pid' options is required. Exiting.", 'warning');
    return;
  }
  $log = drush_get_option('logfile', FALSE);
  $errors = array();
  $list = array();

  if (isset($pid)){
    $list = array_merge($list, array(array('pid' => array('value' => $pid))));
  }
  if(isset($namespace)) {
    $list = array_merge($list, get_namespace_members($namespace));
  }
  foreach ($list as $data) {
    $pid = $data['pid']['value'];
    $object = islandora_object_load($pid);
    if(!$object) {
      drush_log("No object found: $pid.", 'warning');
      $errors[] = $pid;
    }
    else {
      $object->state = $state;
      drush_log("Set state for $pid to $state.", 'success');
    }

    if($log && !empty($errors)) {
      $logdata = "No object found for the following.\n" . implode("\n",$errors);
      file_put_contents($log, $logdata);
      drush_log("Some pids could not be found; see log file at $log.", 'warning');
    }
  }
}

function drush_islandora_utils_get_pids_for_collection() {
  require_once('includes/util.inc');
  $pid = drush_get_option('collection');
  $pids = get_collection_members($pid);
  output_result($pids);
}

/**
 *
 */
function drush_islandora_utils_get_cdm_pointers_for_collection() {
  require_once 'includes/util.inc';
  $pid  = drush_get_option('collection');
  $ptrs = drush_get_option('infile', FALSE) ? explode(',', file_get_contents(drush_get_option('infile'))) : FALSE;
  $ptrs = $ptrs ? array_map('trim', $ptrs) : $ptrs;
  $pids = get_collection_members($pid);
  $map  = array();
  $hits = array();

  foreach ($pids as $pid) {
    $obj   = islandora_object_load($pid);
    $mods  = $obj->getDatastream('MODS')->content;
    $xml   = simplexml_load_string($mods);
    $ptr   = $xml->extension->CONTENTdmData->pointer;
    if ($ptrs) {
      if (in_array($ptr, $ptrs)) {
        $map[]  = $pid;
        $hits[] = $ptr;
      }
    }
    else {
      $map[] = sprintf("%s => %s", $ptr, $pid);
    }
  }
  if ($ptrs) {
    output_result(array_diff($ptrs, $hits));
    printf("\nLooking for %d pointers, found %d\nNot found:\n%s\n\n", count($ptrs), count($hits), implode(',', array_diff($ptrs, $hits)));
  }
  else {
    output_result($map);
  }
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

function drush_islandora_utils_index_list() {
  require_once('includes/util.inc');
  $infile = drush_get_option('infile');
  $list = drush_get_option('list');
  if ($infile && $list) {
    throw new Exception("'infile' and 'list' are mutually exclusive options.");
  }
  elseif (!$list && !$infile) {
    throw new Exception('One of the options [infile,list] is required.');
  }
  elseif (!$list) {
    $list = file_get_contents($infile);
  }
  $pids = explode(',', $list);
  index_pids($pids);
}

function drush_islandora_utils_index_namespace () {
  require_once 'includes/util.inc';
  $namespace = drush_get_option('namespace');
  $records = get_namespace_members($namespace);
  $pids = array_map(function($e){ return $e['pid']['value'];}, $records);
  index_pids($pids);
}

function drush_islandora_utils_get_objects_missing_dsid() {
  require_once('includes/util.inc');
  $coll = drush_get_option('collection');
  $dsid = drush_get_option('dsid');
  $pids = get_collection_members($coll);
  $miss = array();

  foreach ($pids as $pid) {
    $object = islandora_object_load($pid);

    if (!$object->getDatastream($dsid)) {
      $miss[] = $pid;
      drush_log("Missing dsid $dsid for $pid", "warning");
      continue;
    }
    drush_log("$pid ok.", "success");
  }
  if (!empty($miss)) {
    drush_log(sprintf("%d items missing the %s datastream.\n", count($miss), $dsid) . output_result($miss) . "\n", 'warning');
  }
  else {
    drush_log("All objects in $coll have datastream $dsid", "success");
  }
}

function drush_islandora_utils_get_objects_changed_dsid(){
  require_once('includes/util.inc');
  module_load_include('inc', 'islandora', 'includes/datastream.version');
  $coll = drush_get_option('collection');
  $dsid = drush_get_option('dsid');
  $outfile = drush_get_option('outfile');
  $pids = get_collection_members($coll);
  drush_log(sprintf("found %s pids", count($pids)), LogLevel::DEBUG);
  $pid_histories = array();
  foreach($pids as $pid) {
    drush_log(sprintf("checking %s for %s", $dsid, $pid), LogLevel::DEBUG);
    $islandora_object = islandora_object_load($pid);
    $datastream = islandora_datastream_load($dsid, $islandora_object);
    if (!$islandora_object[$dsid]) {
      drush_log("$dsid NOT FOUND for $pid", LogLevel::DEBUG);
      continue;
    }
    # $audit_values include item versions that were deleted, etc.
    # But all $datastream values are active versions.  
    # So, we loop over the $datastream items, and pull in 
    # extra information from the matching #audit_values item.
    $rows = array();
    $audit_values = islandora_get_audit_trail($pid, $dsid);
    foreach ($datastream as $version => $datastream_version) {
      $responsibility = $islandora_object->owner;    
      # finds a better author name from audit_value, if possible.
      foreach ($audit_values as $audit_value) {
        if ($audit_value['date'] == $datastream_version->createdDate) {
          $responsibility = strval($audit_value['responsibility']);
          break;
        }
      }
      $edited_date = $datastream_version->createdDate->format(DATE_RFC850);
      foreach ($rows as &$item) {
        if ($item['pid'] == ($pid . "." . $dsid)) {
          if (strval($item['user']) == strval($responsibility)) {
            array_push($item['time'], $edited_date);
            continue 2;
          }
        }
      }
      $row = array(
        'pid' => $pid . "." . $dsid, 
        'user' => $responsibility,
        'time' => array($edited_date),
        );
      $rows[] = $row;
      }
    $pid_histories[] = $rows;
  }
  $toFile = json_encode($pid_histories);
  file_put_contents($outfile, $toFile.PHP_EOL , FILE_APPEND );
  foreach ($pid_histories as $counter=>$per_pid) {
    $notify = false;
    $editors_count = count($per_pid);
    if ($editors_count > 1) {
      $notify = true;
    }
    foreach ($per_pid as $per_editor) {
      $editor = $per_editor['user'];
      $pid = $per_editor['pid'];
      $times_array = $per_editor['time'];
      $times_count = count($times_array);
      $times_string = implode('; ', $times_array);
      if ($times_count > 1) {
        $notify = true;
      }
      if ($notify) {
        drush_log("$dsid for $pid has $times_count changes by $editor on $times_string", LogLevel::WARNING);
      }
    } 
    if ($notify) {
      drush_log("$dsid for $pid has $editors_count editors", LogLevel::WARNING);
    }
  }
}

function drush_islandora_utils_create_collection() {
  require_once('includes/util.inc');
  $namespace = drush_get_option('namespace');
  $parent = drush_get_option('parent');
  $title = drush_get_option('title');
  $descript = drush_get_option('description');

  if (islandora_object_load($namespace)) {
    drush_log("$namespace has already been ingested.\n", "success");
  }
  else {
    islandora_utils_ingest_collection($title, $descript, $namespace, $parent);
  }
}
