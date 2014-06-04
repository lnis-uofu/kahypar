/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include <gmock/gmock.h>

#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/Partitioner.h"
#include "partition/coarsening/HeuristicHeavyEdgeCoarsener.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/coarsening/Rater.h"
#include "partition/refinement/IRefiner.h"
#include "partition/refinement/TwoWayFMRefiner.h"
#include "partition/refinement/policies/FMQueueCloggingPolicies.h"
#include "partition/refinement/policies/FMQueueSelectionPolicies.h"
#include "partition/refinement/policies/FMStopPolicies.h"

using::testing::Test;
using::testing::Eq;
using::testing::DoubleEq;

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeWeight;

using partition::Rater;
using partition::FirstRatingWins;
using partition::ICoarsener;
using partition::IRefiner;
using partition::HeuristicHeavyEdgeCoarsener;
using partition::Configuration;
using partition::Partitioner;
using partition::TwoWayFMRefiner;
using partition::NumberOfFruitlessMovesStopsSearch;
using partition::EligibleTopGain;
using partition::RemoveOnlyTheCloggingEntry;

namespace metrics {
typedef Rater<defs::RatingType, FirstRatingWins> FirstWinsRater;
typedef HeuristicHeavyEdgeCoarsener<FirstWinsRater> FirstWinsCoarsener;
typedef TwoWayFMRefiner<NumberOfFruitlessMovesStopsSearch,
                        EligibleTopGain, RemoveOnlyTheCloggingEntry> Refiner;

class AnUnPartitionedHypergraph : public Test {
  public:
  AnUnPartitionedHypergraph() :
    hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, /*sentinel*/ 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }) { }

  Hypergraph hypergraph;
};

class TheDemoHypergraph : public AnUnPartitionedHypergraph {
  public:
  TheDemoHypergraph() : AnUnPartitionedHypergraph() { }
};

class APartitionedHypergraph : public Test {
  public:
  APartitionedHypergraph() :
    hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, /*sentinel*/ 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }),
    config(),
    partitioner(config),
    coarsener(new FirstWinsCoarsener(hypergraph, config)),
    refiner(new Refiner(hypergraph, config)) {
    config.coarsening.minimal_node_count = 2;
    config.coarsening.threshold_node_weight = 5;
    config.partitioning.graph_filename = "Test";
    config.partitioning.graph_partition_filename = "Test.hgr.part.2.KaHyPar";
    config.partitioning.coarse_graph_filename = "test_coarse.hgr";
    config.partitioning.coarse_graph_partition_filename = "test_coarse.hgr.part.2";
    config.partitioning.epsilon = 0.15;
    partitioner.partition(hypergraph, *coarsener, *refiner);
  }

  Hypergraph hypergraph;
  Configuration config;
  Partitioner partitioner;
  std::unique_ptr<ICoarsener> coarsener;
  std::unique_ptr<IRefiner> refiner;
};

class TheHyperedgeCutCalculationForInitialPartitioning : public AnUnPartitionedHypergraph {
  public:
  TheHyperedgeCutCalculationForInitialPartitioning() :
    AnUnPartitionedHypergraph(),
    config(),
    coarsener(hypergraph, config),
    hg_to_hmetis(),
    partition() {
    config.coarsening.minimal_node_count = 2;
    config.coarsening.threshold_node_weight = 5;
    config.partitioning.graph_filename = "cutCalc_test.hgr";
    config.partitioning.graph_partition_filename = "cutCalc_test.hgr.part.2.KaHyPar";
    config.partitioning.coarse_graph_filename = "cutCalc_test_coarse.hgr";
    config.partitioning.coarse_graph_partition_filename = "cutCalc_test_coarse.hgr.part.2";
    config.partitioning.epsilon = 0.15;
    hg_to_hmetis[1] = 0;
    hg_to_hmetis[3] = 1;
    partition.push_back(1);
    partition.push_back(0);
  }

  Configuration config;
  FirstWinsCoarsener coarsener;
  std::unordered_map<HypernodeID, HypernodeID> hg_to_hmetis;
  std::vector<PartitionID> partition;
};

TEST_F(TheHyperedgeCutCalculationForInitialPartitioning, ReturnsCorrectResult) {
  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph.nodeDegree(1), Eq(1));
  ASSERT_THAT(hypergraph.nodeDegree(3), Eq(1));
  hypergraph.changeNodePartition(1, Hypergraph::kInvalidPartition, 0);
  hypergraph.changeNodePartition(3, Hypergraph::kInvalidPartition, 1);

  ASSERT_THAT(hyperedgeCut(hypergraph, hg_to_hmetis, partition), Eq(hyperedgeCut(hypergraph)));
}

TEST_F(AnUnPartitionedHypergraph, HasHyperedgeCutZero) {
  ASSERT_THAT(hyperedgeCut(hypergraph), Eq(0));
}

TEST_F(APartitionedHypergraph, HasCorrectHyperedgeCut) {
  ASSERT_THAT(hyperedgeCut(hypergraph), Eq(2));
}

TEST_F(TheDemoHypergraph, HasAvgHyperedgeDegree3) {
  ASSERT_THAT(avgHyperedgeDegree(hypergraph), DoubleEq(3.0));
}

TEST_F(TheDemoHypergraph, HasAvgHypernodeDegree12Div7) {
  ASSERT_THAT(avgHypernodeDegree(hypergraph), DoubleEq(12.0 / 7));
}
} // namespace metrics
