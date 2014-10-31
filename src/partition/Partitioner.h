/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_PARTITIONER_H_
#define SRC_PARTITION_PARTITIONER_H_

#include <cstdlib>

#include <algorithm>
#include <chrono>
#include <limits>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "lib/definitions.h"
#include "lib/io/HypergraphIO.h"
#include "lib/io/PartitioningOutput.h"
#include "lib/utils/Stats.h"
#include "partition/Configuration.h"
#include "partition/coarsening/HypergraphPruner.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/refinement/TwoWayFMRefiner.h"
#include "tools/RandomFunctions.h"

#ifndef NDEBUG
#include "partition/Metrics.h"
#endif

using defs::Hypergraph;
using defs::HypernodeWeight;
using defs::HyperedgeWeight;
using defs::PartitionID;
using defs::HypernodeID;
using utils::Stats;

namespace partition {
static const bool dbg_partition_large_he_removal = false;
static const bool dbg_partition_large_he_restore = false;
static const bool dbg_partition_initial_partitioning = true;
static const bool dbg_partition_vcycles = true;

class Partitioner {
  typedef std::unordered_map<HypernodeID, HypernodeID> CoarsenedToHmetisMapping;
  typedef std::vector<HypernodeID> HmetisToCoarsenedMapping;
  typedef std::vector<HypernodeWeight> PartitionWeights;

  enum { kInitialParallelHEremoval = 0 };
  enum { kInitialLargeHEremoval = 1 };
  enum { kCoarsening = 2 };
  enum { kInitialPartitioning = 3 };
  enum { kUncoarseningRefinement = 4 };
  enum { kInitialLargeHErestore = 5 };
  enum { kInitialParallelHErestore = 6 };

  public:
  explicit Partitioner(Configuration& config) :
    _config(config),
    _stats(),
    _timings() { }

  void partition(Hypergraph& hypergraph, ICoarsener& coarsener, IRefiner& refiner);

  const std::array<std::chrono::duration<double>, 7> & timings() const {
    return _timings;
  }

  private:
  FRIEND_TEST(APartitionerWithHyperedgeSizeThreshold, RemovesHyperedgesExceedingThreshold);
  FRIEND_TEST(APartitionerWithHyperedgeSizeThreshold, RestoresHyperedgesExceedingThreshold);
  FRIEND_TEST(APartitionerWithHyperedgeSizeThreshold,
              PartitionsUnpartitionedHypernodesAfterRestore);
  FRIEND_TEST(APartitionerWithHyperedgeSizeThreshold,
              AssignsAllRemainingHypernodesToDefinedPartition);
  FRIEND_TEST(APartitionerWithHyperedgeSizeThreshold,
              TriesToMinimizesCutIfNoPinOfRemainingHyperedgeIsPartitioned);
  FRIEND_TEST(APartitionerWithHyperedgeSizeThreshold,
              TriesToMinimizesCutIfOnlyOnePartitionIsUsed);
  FRIEND_TEST(APartitionerWithHyperedgeSizeThreshold,
              DistributesAllRemainingHypernodesToMinimizeImbalaceIfCutCannotBeMinimized);

  typedef  std::stack<std::pair<int, int> > RemovedParallelHyperedgesStack;

  void removeLargeHyperedges(Hypergraph& hg, std::vector<HyperedgeID>& removed_hyperedges);
  void restoreLargeHyperedges(Hypergraph& hg, std::vector<HyperedgeID>& removed_hyperedges);
  void partitionUnpartitionedPins(HyperedgeID he, Hypergraph& hg);
  void assignUnpartitionedPinsToPartition(HyperedgeID he, PartitionID id, Hypergraph& hg);
  void assignAllPinsToPartition(HyperedgeID he, PartitionID id, Hypergraph& hg);
  void distributePinsAcrossPartitions(HyperedgeID he, Hypergraph& hg);
  void createMappingsForInitialPartitioning(HmetisToCoarsenedMapping& hmetis_to_hg,
                                            CoarsenedToHmetisMapping& hg_to_hmetis,
                                            const Hypergraph& hg);
  void performInitialPartitioning(Hypergraph& hg);
  void removeParallelHyperedges(Hypergraph& hypergraph, HypergraphPruner& hypergraph_pruner,
                                RemovedParallelHyperedgesStack& stack);
  void restoreParallelHyperedges(Hypergraph& hypergraph, HypergraphPruner& hypergraph_pruner,
                                 RemovedParallelHyperedgesStack& stack);

  Configuration& _config;
  Stats _stats;
  std::array<std::chrono::duration<double>, 7> _timings;
};
} // namespace partition

#endif  // SRC_PARTITION_PARTITIONER_H_
