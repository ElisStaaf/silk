#!/bin/bash

SILK_BASE_SRC=src/silk.h
SILK_BASE_DEST=/usr/include/silk.h
SILK_EXTE_SRC=src/silk
SILK_EXTE_DEST=/usr/include/silk

sudo cp -r \
    $SILK_BASE_SRC \
    $SILK_BASE_DEST
sudo cp -r \
    $SILK_EXTE_SRC \
    $SILK_EXTE_DEST
