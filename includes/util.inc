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
function islandora_utils_ingest_collection($title, $descript, $namespace = 'islandora', $parent = 'islandora:root', $cmodels = array(), $user = NULL) {
  try {
    $tuque = islandora_get_tuque_connection($user);
    $collection_object = $tuque->repository->constructObject($namespace);
    $collection_object->label = $title;

    if (!empty($cmodels)) {
      $cmodel_xml = '';
      foreach($cmodels as $cm) {
        $cmodel_xml .= sprintf("<content_model dsid=\"%s\" name=\"%s\" namespace=\"%s\" pid=\"%s\"/>",
            $cm['dsid'],
            $cm['name'],
            $cm['namespace'],
            $cm['pid']
            );
      }
    }
    else {
      $cmodel_xml = '';
    }
    $collection_policy_xml = <<<EOCP
<collection_policy xmlns="http://www.islandora.ca" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" name="$title" xsi:schemaLocation="http://www.islandora.ca http://syn.lib.umanitoba.ca/collection_policy.xsd">
<content_models>$cmodel_xml</content_models>
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

    // Borrowed from mjordan remote resource soln. pack.
    $path_to_mods_to_dc_xsl = file_get_contents(drupal_get_path('module', 'islandora_batch') . '/transforms/mods_to_dc.xsl');
    $dc_xml = apply_xslt($path_to_mods_to_dc_xsl, $mods_datastream->content);
    $dc_datastream = $collection_object->constructDatastream('DC', 'M');
    $dc_datastream->label = 'DC Record';
    $dc_datastream->mimetype = 'application/xml';
    $dc_datastream->setContentFromString($dc_xml);
    $collection_object->ingestDatastream($dc_datastream);

    // Handle TN.
    $path_to_thumbnail = drupal_get_path('module', 'islandora_basic_collection') . '/images/folder.png';
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

    $tuque->repository->ingestObject($collection_object);
    drupal_set_message(t('Ingested Islandora collection object %t (PID %p).',
      array('%t' => $collection_object->label, '%p' => $collection_object->id)));
    watchdog('islandora_utils', 'Ingested Islandora collection object %t (PID %p).',
      array('%t' => $collection_object->label, '%p' => $collection_object->id), WATCHDOG_INFO);
  }
  catch (Exception $e) {
    drupal_set_message(t('Error ingesting Islandora collection object %t (PID %p).',
      array('%t' => $collection_object->label, '%p' => $collection_object->id)), 'error');
    watchdog('islandora_utils', 'Error ingesting Islandora collection object %t (PID %p).',
      array('%t' => $title, '%p' => $collection_object->id), WATCHDOG_ERROR);
  }
}

/**
 * Applies an XSLT transform to an XML string.
 *
 * @param string $xslt
 *   An XSLT stylesheet.
 * @param string $input_xml
 *   An XML string.
 * @param array $params
 *   An associative array of parameters to the stylesheet.
 *
 * @return string
 *   The transformed XML.
 */
function apply_xslt($xslt, $input_xml, $params = array()) {
  try {
    $xsl_doc = new DOMDocument();
    $xsl_doc->loadXML($xslt);
    $xml_doc = new DOMDocument();
    $xml_doc->loadXML($input_xml);
    $xslt_proc = new XSLTProcessor();
    $xslt_proc->importStylesheet($xsl_doc);
    if (count($params)) {
      foreach ($params as $param_name => $param_value) {
        $xslt_proc->setParameter(NULL, $param_name, check_plain($param_value));
      }
    }
    $output_xml = $xslt_proc->transformToXML($xml_doc);
    return $output_xml;
  }
  catch (exception $e) {
    $success = array(
      'success' => FALSE,
      'messages' => array(
        array(
          'message' => t('Failed to apply XSL transform.'),
          'type' => 'watchdog',
          'severity' => WATCHDOG_ERROR,
        ),
      ),
    );
  }
}

/**
 * @params associative array that will be extract() -ed.
 */
function islandora_utils_get_mods($params) {
  extract($params);
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
<typeOfResource />
<genre authority="lctgm"/>
<originInfo>
  <dateCreated>$date</dateCreated>
</originInfo>
<language>
  <languageTerm authority="iso639-2b" type="code">eng</languageTerm>
</language>
<abstract>$description</abstract>
<identifier type="local">$pid</identifier>
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
  return $mods_xml;
}
