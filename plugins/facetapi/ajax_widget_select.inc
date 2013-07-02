<?php

/**
 * @file
 * The facetapi_links and facetapi_checkbox_links widget plugin classes.
 */

/**
 * Widget that renders facets as a list of clickable links.
 *
 * Links make it easy for users to narrow down their search results by clicking
 * on them. The render arrays use theme_item_list() to generate the HTML markup.
 */
class FacetapiAjaxWidgetSelect extends FacetapiWidgetCheckboxLinks {

  /**
   * Overrides FacetapiWidgetCheckboxLinks::init().
   */
  public function init() {
    parent::init();
    drupal_add_js(
      array(
        'facetapi' => array('ajax_select_box' => array('default_value' => t('Выберите')))
      ),
      'setting'
    );
  }

  /**
   * Overrides FacetapiWidget::settingsForm().
   */
  function settingsForm(&$form, &$form_state) {
    $form['widget']['widget_settings']['links'][$this->id]['index_id'] = array(
      '#type' => 'value',
      '#title' => t('Index id'),
      '#default_value' => $form['#facetapi']['facet']['map options']['index id'],
      '#description' => t('Limits the number of displayed facets via JavaScript.'),
      '#states' => array(
        'visible' => array(
          'select[name="widget"]' => array('value' => $this->id),
        ),
      ),
    );
  }

  /**
   * Transforms the render array for use with theme_item_list().
   *
   * The recursion allows this function to act on the various levels of a
   * hierarchical data set.
   *
   * @param array $build
   *   The items in the facet's render array being transformed.
   *
   * @return array
   *   The "items" parameter for theme_item_list().
   */
  function buildListItems($build) {

    $have_active = FALSE;
    // Builds rows.
    $items = array();

    // Put default value as JS setting.
    drupal_add_js(
      array(
        'facetapi' => array('ajax_select_box' => array('default_value' => t('Select')))
      ),
      'setting'
    );

    $items['values'][0] = t('Select');

    $active_items = array();
    foreach ($build as $value => $item) {
      // Respect current selection.
      if ($item['#active']) {
        $attributes['checked'] = 'checked';
        $have_active = TRUE;
        $items['active_value'] = $value;
        $active_items[] = $this->key . ':' . $value;
      }

      if ($term = taxonomy_term_load($item['#indexed_value'])) {
        $items['values'][$item['#indexed_value']] = $term->name;

        // Get name for facet.
        if (empty($items['name'])) {
          if ($name_tid = field_get_items('taxonomy_term', $term, 'field_property')) {
            if ($name_term = taxonomy_term_load($name_tid[0]['tid'])) {
              $items['name'] = $name_term->name;
            }
          }
        }
      }
      else if ($item['#indexed_value'] != '!') {
        $items['values'][$item['#indexed_value']] = $item['#indexed_value'];
      }
    }

    $this->jsSettings['haveActiveSelection'] = $this->settings->settings['have_active_selection'] = $have_active;
    sort($active_items);
    $this->jsSettings['activeItems'] = $active_items;
    // Generate reset path on server side to make possible to use aliases.
    if ($have_active) {
      $this->jsSettings['resetPath'] = ajax_facets_facet_build_reset_path($this->facet->getFacet(), $this->facet->getAdapter());
    }
    return $items;
  }

  /**
   * Implements FacetapiWidget::execute().
   *
   * Transforms the render array into something that can be themed by
   * theme_item_list().
   *
   * @see FacetapiWidgetLinks::setThemeHooks()
   * @see FacetapiWidgetLinks::buildListItems()
   */
  public function execute() {
    $element = &$this->build[$this->facet['field alias']];

    $items = $this->buildListItems($element);

    // If name is empty - use label.
    if (empty($items['name'])) {
      $facet = $this->facet->getFacet();
      $items['name'] = $facet['label'];
    }

    $select = array(
      '#type' => 'select',
      '#title' => $this->build['#title'],
      '#options' => $items['values'],
      '#name' => urlencode($this->settings->facet),
      '#attributes' => array(
        'data-facet' => urlencode($this->settings->facet),
      ),
    );

    if (!empty($items['active_value'])) {
      $select['#value'] = $items['active_value'];
    }

    $facet = $this->facet->getSettings();
    $delta = facetapi_hash_delta(facetapi_build_delta($facet->searcher, 'block', $facet->facet));
    $id = 'block-facetapi-' . strtolower($delta);

    $element = array(
      '#markup' => '<div id="' . $id . '" class="facet-wrapper-selectbox">'
        . '<div id="' . $this->build['#attributes']['id'] . '">' . render($select) . '</div>'
        . '</div>'
    );
  }
}