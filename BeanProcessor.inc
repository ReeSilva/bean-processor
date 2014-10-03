<?php

/**
 * @file
 * Class definition of FeedsNodeProcessor.
 */

/**
 * Creates blocks from feed items.
 */

class BeanProcessor extends FeedsProcessor {


    /**
     * Entity type this processor operates on.
     */
    public function entityType()
    {
        return 'bean';
    }

    /**
     * Implements parent::entityInfo().
     */
    protected function entityInfo() {
        $info = parent::entityInfo();
        $info['label plural'] = t('Beans');
        return $info;
    }

    /**
     * Create a new entity.
     *
     * @param FeedsSource $source
     *   The feeds source that spawns this entity.
     *
     * @return A new entity object.
     */
    protected function newEntity(FeedsSource $source)
    {
        $bean = new Bean();
        $bean->type = $this->bundle();
        $bean->changed = REQUEST_TIME;
        $bean->created = REQUEST_TIME;
        $bean->loadUp(bean_load_plugin_class($this->bundle()));
        $bean->uid = $this->config['author'];

        return $bean;
    }

    protected function entitySaveAccess($entity) {

        // The check will be skipped for anonymous nodes.
        if ($this->config['authorize'] && !empty($entity->uid)) {

            $author = user_load($entity->uid);

            // If the uid was mapped directly, rather than by email or username, it
            // could be invalid.
            if (!$author) {
                $message = 'User %uid is not a valid user.';
                throw new FeedsAccessException(t($message, array('%uid' => $entity->uid)));
            }

            if (empty($entity->nid) || !empty($entity->is_new)) {
                $op = 'create';
                $access = bean_access($op, $entity->type, $author);
            }
            else {
                $op = 'update';
                $access = bean_access($op, $entity, $author);
            }

            if (!$access) {
                $message = 'User %name is not authorized to %op content type %content_type.';
                throw new FeedsAccessException(t($message, array('%name' => $author->name, '%op' => $op, '%content_type' => $entity->type)));
            }
        }
    }

    protected function entityValidate($entity) {
        if (!isset($entity->uid) || !is_numeric($entity->uid)) {
            $entity->uid = $this->config['author'];
        }
    }

    /**
     * Save an entity.
     *
     * @param $entity
     *   Entity to be saved.
     */
    protected function entitySave($entity)
    {
        bean_save($entity);
    }

    /**
     * Delete a series of entities.
     *
     * @param $entity_ids
     *   Array of unique identity ids to be deleted.
     */
    protected function entityDeleteMultiple($entity_ids)
    {
        // TODO: Implement entityDeleteMultiple() method.
    }

    /**
     * Override parent::configDefaults().
     */
    public function configDefaults() {
        return array(
            'expire' => FEEDS_EXPIRE_NEVER,
            'author' => 0,
            'authorize' => TRUE,
        ) + parent::configDefaults();
    }

    public function configForm(&$form_state) {
        $form = parent::configForm($form_state);

        $author = user_load($this->config['author']);
        $form['author'] = array(
            '#type' => 'textfield',
            '#title' => t('Author'),
            '#description' => t('Select the author of the nodes to be created - leave empty to assign "anonymous".'),
            '#autocomplete_path' => 'user/autocomplete',
            '#default_value' => empty($author->name) ?  'anonymous' : check_plain($author->name),
        );
        $form['authorize'] = array(
            '#type' => 'checkbox',
            '#title' => t('Authorize'),
            '#description' => t('Check that the author has permission to create the node.'),
            '#default_value' => $this->config['authorize'],
        );

        return $form;
    }

    public function configFormValidate(&$values) {
        if ($author = user_load_by_name($values['author'])) {
            $values['author'] = $author->uid;
        }
        else {
            $values['author'] = 0;
        }
    }

    /**
     * Reschedule if expiry time changes.
     */
    public function configFormSubmit(&$values) {
        if ($this->config['expire'] != $values['expire']) {
            feeds_reschedule($this->id);
        }
        parent::configFormSubmit($values);
    }

    public function getMappingTargets() {
        $targets = parent::getMappingTargets();

        $targets['label'] = array (
            'name' => t('Label'),
            'description' => t('The label of the block'),
            'optional_unique' => TRUE,
        );

        // Let other modules expose mapping targets.
        self::loadMappers();
        $entity_type = $this->entityType();
        $bundle = $this->bundle();
        drupal_alter('feeds_processor_targets', $targets, $entity_type, $bundle);

        return $targets;
    }
}
