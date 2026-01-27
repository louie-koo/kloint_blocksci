Parser TX Data Corruption Analysis
===================================

This document describes a critical data corruption issue discovered in the BlockSci parser
and the fixes implemented to prevent it.

Problem Summary
---------------

When querying addresses, the expected number of transactions was not returned.
For example, address ``3FzAoX2s7GhrZUZR4WXs1CvTKUGd7Ze6YM`` should have 188 transactions,
but BlockSci only returned 2.

Investigation revealed that **chain data itself was corrupted** - TX data and block metadata
were desynchronized.

Symptoms
--------

The corruption manifests as transactions being mapped to incorrect blocks:

+------------------+-------------------+---------+
| BlockSci Block   | Actual TX Block   | Offset  |
+==================+===================+=========+
| 500000           | 415436            | +84,564 |
+------------------+-------------------+---------+
| 600000           | 523347            | +76,653 |
+------------------+-------------------+---------+
| 700000           | 625810            | +74,190 |
+------------------+-------------------+---------+
| 816723           | 757392            | +59,331 |
+------------------+-------------------+---------+
| 858683           | 816723            | +41,960 |
+------------------+-------------------+---------+

The decreasing offset indicates this is not a simple offset problem, but a
**desynchronization between TX data and block data**.

Corruption Location
-------------------

**Exact corruption start point**:

+----------------------+------------------+
| Parameter            | Value            |
+======================+==================+
| Block Height         | 389,206          |
+----------------------+------------------+
| firstTxIndex         | 98,803,290       |
+----------------------+------------------+
| Approximate TX count | ~100 million     |
+----------------------+------------------+
| Parser Batch         | 2nd batch end    |
+----------------------+------------------+

The corruption occurs near the boundary of the second batch (batch size: 50,000,000 TXs).

Data State Analysis
-------------------

- ``block.dat``: Block hashes correct, TX counts correct
- ``tx_data.dat`` + ``tx_index.dat``: TX stored at wrong positions
- ``firstTxIndex``: Continuous (no gaps)

Root Cause
----------

The most likely cause is **incomplete recovery after parser interruption**.

Scenario
~~~~~~~~

1. Initial parsing: Blocks 0 ~ 389205 processed normally
2. Interruption occurs during or after block 389206
3. On resume:

   - ``block.dat`` retains previous state
   - New TXs appended to ``tx_data.dat`` (after existing data)
   - However, ``block.firstTxIndex`` retains old values

4. Result: TXs mapped to wrong blocks

Code Vulnerabilities (Fixed)
----------------------------

The following vulnerabilities were identified and fixed:

1. TX File Flush Missing
~~~~~~~~~~~~~~~~~~~~~~~~

**Location**: ``tools/parser/main.cpp:208``

Before fix::

    blockFile.flush();  // Only block file flushed
    // TX file not flushed!

After fix::

    blockFile.flush();
    // backUpdateTxes now flushes TX files internally
    backUpdateTxes(config);

2. Incomplete Checkpoint
~~~~~~~~~~~~~~~~~~~~~~~~

**Location**: ``tools/parser/main.cpp:210-216``

Before fix::

    backUpdateTxes(config);  // Modifies TX file
    utxoAddressState.serialize(...);  // Saves state
    // Checkpoint completed without TX file flush

After fix::

    backUpdateTxes(config);  // Now flushes internally
    // Atomic serialization with temp file + rename
    serializeAtomically(utxoAddressState, path);
    serializeAtomically(utxoState, path);
    serializeAtomically(utxoScriptState, path);

3. No Validation on Resume
~~~~~~~~~~~~~~~~~~~~~~~~~~

**Location**: ``tools/parser/main.cpp``

Before fix: No validation of existing TX count vs block.firstTxIndex consistency.

After fix: ``verifyChainIntegrity()`` function added to check:

- Last block's ``firstTxIndex + txCount`` matches actual TX file count
- ``firstTxIndex`` continuity across blocks

Fixes Implemented
-----------------

1. Integrity Verification
~~~~~~~~~~~~~~~~~~~~~~~~~

Added ``verifyChainIntegrity()`` function that runs before parsing starts.
If corruption is detected, the parser exits with a clear error message and
recovery instructions.

2. Explicit TX File Flush
~~~~~~~~~~~~~~~~~~~~~~~~~

- ``backUpdateTxes()`` now calls ``txFile.clearBuffer()`` before returning
- ``addNewBlocks()`` flushes all TX-related files before returning
- Added ``NewBlocksFiles::flush()`` method

3. Atomic State Serialization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

All state files are now saved atomically using the temp file + rename pattern::

    // Write to temp file first
    state.serialize(path + ".tmp");
    // Atomic rename (POSIX guarantees atomicity)
    std::rename(tempPath, path);

This ensures state files are never partially written.

Recovery Procedure
------------------

If corruption is detected, the only reliable recovery is to rebuild from scratch:

.. code-block:: bash

    # 1. Backup existing data (optional)
    mv /path/to/blocksci_data /path/to/blocksci_data_backup

    # 2. Create new config and parse from scratch
    blocksci_parser /path/to/new_config.json generate-config \
        bitcoin /path/to/blocksci_data \
        --disk /path/to/bitcoin/datadir

    # 3. Full parsing (must complete without interruption)
    blocksci_parser /path/to/config.json update

    # 4. Build indexes
    blocksci_parser /path/to/config.json hash-index-update
    blocksci_parser /path/to/config.json address-index-update

Prevention
----------

To prevent this issue:

1. **Allow parsing to complete**: Avoid interrupting the parser during batch processing
2. **Use updated parser**: The fixes ensure proper flushing and atomic checkpoints
3. **Verify after parsing**: Use the integrity check built into the parser

Verification
------------

After rebuilding, verify data integrity:

.. code-block:: bash

    # The parser now automatically verifies on startup
    blocksci_parser /path/to/config.json update

    # Should output:
    # "Chain integrity check passed: X transactions in Y blocks"

You can also verify specific addresses against blockchain explorers.

Technical Details
-----------------

Batch Size
~~~~~~~~~~

The parser processes blocks in batches of ~50,000,000 transactions.
The corruption started near the end of the second batch (~98,803,290 TXs),
suggesting a batch boundary issue.

Files Affected
~~~~~~~~~~~~~~

+----------------------+------------------------------------------+
| File                 | Description                              |
+======================+==========================================+
| chain/block.dat      | Block metadata including firstTxIndex    |
+----------------------+------------------------------------------+
| chain/tx_data.dat    | Transaction data                         |
+----------------------+------------------------------------------+
| chain/tx_index.dat   | TX offset index                          |
+----------------------+------------------------------------------+
| chain/tx_hashes.dat  | TX hash mapping                          |
+----------------------+------------------------------------------+

Related Code Changes
~~~~~~~~~~~~~~~~~~~~

- ``tools/parser/main.cpp``: Added integrity check, atomic serialization
- ``tools/parser/block_processor.cpp``: Added explicit flushes
- ``tools/parser/block_processor.hpp``: Added flush() method to NewBlocksFiles
