<?php
require_once('gsearch.inc');

function index_pids($list){

    $item_count = count($list);
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
        $cnt = count($failures);
        drush_log("$cnt items failed or timed-out during indexing\n". output_result($failures), 'warning');
    }else{
        drush_log("All ($item_count) items were reindexed successfully.", "success");
    }
}

function csv_format_pid_list($pids){
    if(!is_array($pids)){
        throw new Exception('Input param is not an array.');
    }
    return implode(',',$pids);
}

/**
 * 
 * @param array $pids
 */
function output_result($pids){
    $file = drush_get_option('outfile');
    $pidstr = csv_format_pid_list($pids);
    if($file){
        print("See file $file\n");
        file_put_contents($file, $pidstr);
    }else{
        print("\n".$pidstr."\n\n");
    }
}

/**
 * Ingest an Islandora collection object.
 *
 * Modified from https://github.com/mjordan/islandora_migrate_cdm_collections
 *
 *
 * @param string $namespace
 *   The Fedora namespace to use for this object.
 * @param string $parent
 *   The PID of the parent collection the newly created collection.
 */
function islandora_utils_ingest_collection($title, $descript, $namespace = 'islandora', $parent = 'islandora:root') {

  try {
    $tuque = new IslandoraTuque();
    $repository = $tuque->repository;
    $collection_object = $repository->constructObject($namespace);
    $collection_object->label = $title;

    $collection_policy_xml = <<<EOCP
<collection_policy xmlns="http://www.islandora.ca" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" name="$title" xsi:schemaLocation="http://www.islandora.ca http://syn.lib.umanitoba.ca/collection_policy.xsd">
<content_models/>
<search_terms/>
<staging_area/>
<relationship>isMemberOfCollection</relationship>
</collection_policy>
EOCP;

    // Add the COLLECTION_POLICY ds.
    $coll_policy_datastream = $collection_object->constructDatastream('COLLECTION_POLICY', 'M');
    $coll_policy_datastream->label = 'Collection policy';
    $coll_policy_datastream->mimetype = 'text/xml';
    $coll_policy_datastream->setContentFromString($collection_policy_xml);
    $collection_object->ingestDatastream($coll_policy_datastream);

    // Add the MODS ds.
    $collection_description = $descript;

    $mods_xml = <<<EODC
<?xml version="1.0"?>
<mods xmlns="http://www.loc.gov/mods/v3" xmlns:mods="http://www.loc.gov/mods/v3" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xlink="http://www.w3.org/1999/xlink">
  <titleInfo>
    <title>$title</title>
  </titleInfo>
  <name type="personal">
    <namePart/>
    <role>
      <roleTerm authority="marcrelator" type="text"/>
    </role>
  </name>
  <typeOfResource collection="yes"/>
  <genre authority="lctgm"/>
  <originInfo>
    <dateCreated/>
  </originInfo>
  <language>
    <languageTerm authority="iso639-2b" type="code">eng</languageTerm>
  </language>
  <abstract>$collection_description</abstract>
  <identifier type="local"/>
  <physicalDescription>
    <form authority="marcform"/>
    <extent/>
  </physicalDescription>
  <note/>
  <accessCondition/>
  <subject>
    <topic/>
    <geographic/>
    <temporal/>
    <hierarchicalGeographic>
      <continent/>
      <country/>
      <province/>
      <region/>
      <county/>
      <city/>
      <citySection/>
    </hierarchicalGeographic>
    <cartographics>
      <coordinates/>
    </cartographics>
  </subject>
</mods>
EODC;

    $mods_datastream = $collection_object->constructDatastream('MODS', 'M');
    $mods_datastream->label = 'MODS Record';
    $mods_datastream->mimetype = 'application/xml';
    $mods_datastream->setContentFromString($mods_xml);
    $collection_object->ingestDatastream($mods_datastream);


    // Handle TN.
    $path_to_thumbnail = drupal_get_path('module', 'islandora_basic_collection').'/images/folder.png';
    $tn_datastream = $collection_object->constructDatastream('TN', 'M');
    $tn_mime_detector = new MimeDetect();
    $tn_datastream->mimetype = $tn_mime_detector->getMimetype($path_to_thumbnail);
    $tn_datastream->label = 'Thumbnail';
    $tn_datastream->setContentFromFile($path_to_thumbnail);
    $collection_object->ingestDatastream($tn_datastream);

    // Add relationships.
    $rels = $collection_object->relationships;
    $rels->add('info:fedora/fedora-system:def/relations-external#', 'isMemberOfCollection', $parent, FALSE);
    $rels->add('info:fedora/fedora-system:def/model#', 'hasModel', 'islandora:collectionCModel', FALSE);

    $repository->ingestObject($collection_object);
    drupal_set_message(t('Ingested Islandora collection object %t (PID %p).',
      array('%t' => $collection_object->label, '%p' => $collection_object->id)));
    watchdog('islandora_migrate_cdm_collections', 'Ingested Islandora collection object %t (PID %p).',
      array('%t' => $collection_object->label, '%p' => $collection_object->id), WATCHDOG_INFO);
  }
  catch (Exception $e) {
    drupal_set_message(t('Error ingesting Islandora collection object %t (PID %p).',
      array('%t' => $collection_object->title, '%p' => $collection_object->id)), 'error');
    watchdog('islandora_migrate_cdm_collections', 'Error ingesting Islandora collection object %t (PID %p).',
      array('%t' => $title, '%p' => $collection_object->id), WATCHDOG_ERROR);
  }
}
/**
 * Query for child datastreams on a compound parent (needs to return parent MODS if mods exist.)
 *
 *
 * @param string $parent_pid
 *   The pid for the parent (check that it is a compound)
 * @param string $dsids
 *   The single or multiple dsid to return from the children.
 */

function islandora_utils_gather_children_dsids($parent_pid, $dsids = 'OBJ'){ 
//drupal_get_path('islandora_compound_object','Islandora compound object');
    //module_load_include('inc', 'islandora_compound_object', 'includes/backends');
//  if(!module_load_include('inc', 'islandora_compound_object', 'includes/backends')){
//      exit();
//  }
  try {
//  $tuque = new IslandoraTuque();
//  $repository = $tuque->repository;
  
  $parent_object = islandora_object_load($parent_pid);
  //print_r($parent_object);
  
  $child_objects = islandora_utils_compound_query($parent_pid);
  print_r($child_objects);
   
   
    if(count($dsids) > 1){
        foreach($dsid as $dsids){
          drush_print($dsid);
        }
    }
    else { 
      drush_print($dsids);
    }
  }
  catch (Exception $e) {
    drupal_set_message(t('Error finding parent object %t (PID %p).',
    array('%t' => $parent_object->title, '%p' => $parent_object->id)), 'error');
  }
}
  
  
function islandora_utils_compound_query($pid) {
  $rels_predicate = variable_get('islandora_compound_object_relationship', 'isConstituentOf');
  $objects = array();

  $connection = islandora_get_tuque_connection();
  //print_r($connection);
  if ($connection) {
    $query = <<<EOQ
PREFIX islandora-rels-ext: <http://islandora.ca/ontology/relsext#>
SELECT ?pid
FROM <#ri>
WHERE { ?pid <fedora-rels-ext:isConstituentOf> <info:fedora/$pid> ;}
EOQ;
    $results = $connection->repository->ri->sparqlQuery($query);
    print_r($results);
  }
  return $results;
}