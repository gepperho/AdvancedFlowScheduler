#include <gtest/gtest.h>
#include <solver/Placement.h>
#include <solver/UtilizationList.h>
#include <testUtil.h>
#include <util/Constants.h>

TEST(UtilizationListSearchTransmissionOpportunities, case_slot_completely_before_arrival)
{
    constexpr std::size_t hyper_cycle = 2000;
    auto utilizationList = common::NetworkUtilizationList{1, hyper_cycle, 100};

    // block [50, hyper_cycle]
    utilizationList.reserveSlot(common::SlotReservationRequest{.egress_queue = common::NetworkQueueID{0},
                                                               .start_time = 50,
                                                               .next_slot_start = hyper_cycle,
                                                               .arrival_time = 50},
                                common::FlowNodeID{1}, common::ConfigurationNodeID{1});


    const auto stream = graph_structs::Flow{.id = common::FlowNodeID{2}, .frame_size = 1250, .period = 1000, .source = common::NetworkNodeID{0}, .destination = common::NetworkNodeID{1}};
    const auto config = graph_structs::Configuration{.id = common::ConfigurationNodeID{2}, .flow = stream.id, .path = {common::NetworkQueueID{0}}};

    const auto reservations = utilizationList.searchTransmissionOpportunities(config, stream, 60, 1000);
    EXPECT_TRUE(reservations.empty());
}

TEST(UtilizationListSearchTransmissionOpportunities, case_slot_partially_before_arrival)
{
    constexpr std::size_t hyper_cycle = 2000;
    auto utilizationList = common::NetworkUtilizationList{1, hyper_cycle, 100};

    // block [50, hyper_cycle]
    utilizationList.reserveSlot(common::SlotReservationRequest{.egress_queue = common::NetworkQueueID{0},
                                                               .start_time = 50,
                                                               .next_slot_start = hyper_cycle,
                                                               .arrival_time = 50},
                                common::FlowNodeID{1}, common::ConfigurationNodeID{1});


    const auto stream = graph_structs::Flow{.id = common::FlowNodeID{2}, .frame_size = 1250, .period = 1000, .source = common::NetworkNodeID{0}, .destination = common::NetworkNodeID{1}};
    const auto config = graph_structs::Configuration{.id = common::ConfigurationNodeID{2}, .flow = stream.id, .path = {common::NetworkQueueID{0}}};

    // remaining slot size is not sufficient to send the frame
    auto reservations = utilizationList.searchTransmissionOpportunities(config, stream, 45, 1000);
    EXPECT_TRUE(reservations.empty());
    // remaining slot size is sufficient to send the frame
    reservations = utilizationList.searchTransmissionOpportunities(config, stream, 40, 1000);
    EXPECT_FALSE(reservations.empty());
}

TEST(UtilizationListSearchTransmissionOpportunities, case_slot_at_or_after_arrival)
{
    constexpr std::size_t hyper_cycle = 2000;
    auto utilizationList = common::NetworkUtilizationList{1, hyper_cycle, 100};

    // block [0, 99] and [120, hyper_cycle]
    utilizationList.reserveSlot(common::SlotReservationRequest{.egress_queue = common::NetworkQueueID{0},
                                                               .start_time = 0,
                                                               .next_slot_start = 100,
                                                               .arrival_time = 0},
                                common::FlowNodeID{1}, common::ConfigurationNodeID{1});
    utilizationList.reserveSlot(common::SlotReservationRequest{.egress_queue = common::NetworkQueueID{0},
                                                               .start_time = 120,
                                                               .next_slot_start = hyper_cycle,
                                                               .arrival_time = 120},
                                common::FlowNodeID{1}, common::ConfigurationNodeID{1});


    const auto stream = graph_structs::Flow{.id = common::FlowNodeID{2}, .frame_size = 1250, .period = 1000, .source = common::NetworkNodeID{0}, .destination = common::NetworkNodeID{1}};
    const auto config = graph_structs::Configuration{.id = common::ConfigurationNodeID{2}, .flow = stream.id, .path = {common::NetworkQueueID{0}}};

    // slot starts at arrival (schedulable)
    auto release_time = 100;
    auto reservations = utilizationList.searchTransmissionOpportunities(config, stream, release_time, 1000);

    EXPECT_FALSE(reservations.empty());
    EXPECT_EQ(reservations.begin()->start_time, 100);
    EXPECT_EQ(reservations.begin()->next_slot_start, 110);

    // slot starts before arrival (schedulable)
    release_time = 95;
    reservations = utilizationList.searchTransmissionOpportunities(config, stream, release_time, 1000);
    EXPECT_FALSE(reservations.empty());
    EXPECT_EQ(reservations.begin()->start_time, 100);
    EXPECT_EQ(reservations.begin()->next_slot_start, 110);
}

TEST(UtilizationListSearchTransmissionOpportunities, case_slot_too_small)
{
    constexpr std::size_t hyper_cycle = 2000;
    auto utilizationList = common::NetworkUtilizationList{1, hyper_cycle, 100};

    // block [0, 99] and [105, hyper_cycle]
    utilizationList.reserveSlot(common::SlotReservationRequest{.egress_queue = common::NetworkQueueID{0},
                                                               .start_time = 0,
                                                               .next_slot_start = 100,
                                                               .arrival_time = 0},
                                common::FlowNodeID{1}, common::ConfigurationNodeID{1});
    utilizationList.reserveSlot(common::SlotReservationRequest{.egress_queue = common::NetworkQueueID{0},
                                                               .start_time = 105,
                                                               .next_slot_start = hyper_cycle,
                                                               .arrival_time = 105},
                                common::FlowNodeID{1}, common::ConfigurationNodeID{1});


    const auto stream = graph_structs::Flow{.id = common::FlowNodeID{2}, .frame_size = 1250, .period = 1000, .source = common::NetworkNodeID{0}, .destination = common::NetworkNodeID{1}};
    const auto config = graph_structs::Configuration{.id = common::ConfigurationNodeID{2}, .flow = stream.id, .path = {common::NetworkQueueID{0}}};


    const auto reservations = utilizationList.searchTransmissionOpportunities(config, stream, 50, hyper_cycle);
    EXPECT_TRUE(reservations.empty());
}

TEST(UtilizationListSearchTransmissionOpportunities, case_deadline_during_slot)
{
    constexpr std::size_t hyper_cycle = 2000;
    auto utilizationList = common::NetworkUtilizationList{1, hyper_cycle, 100};

    // block [0, 99] and [120, hyper_cycle]
    utilizationList.reserveSlot(common::SlotReservationRequest{.egress_queue = common::NetworkQueueID{0},
                                                               .start_time = 0,
                                                               .next_slot_start = 100,
                                                               .arrival_time = 0},
                                common::FlowNodeID{1}, common::ConfigurationNodeID{1});
    utilizationList.reserveSlot(common::SlotReservationRequest{.egress_queue = common::NetworkQueueID{0},
                                                               .start_time = 120,
                                                               .next_slot_start = hyper_cycle,
                                                               .arrival_time = 120},
                                common::FlowNodeID{1}, common::ConfigurationNodeID{1});


    const auto stream = graph_structs::Flow{.id = common::FlowNodeID{2}, .frame_size = 1250, .period = 1000, .source = common::NetworkNodeID{0}, .destination = common::NetworkNodeID{1}};
    const auto config = graph_structs::Configuration{.id = common::ConfigurationNodeID{2}, .flow = stream.id, .path = {common::NetworkQueueID{0}}};
    // deadline during slot, transmission fits before
    auto release_time = 100;
    auto reservations = utilizationList.searchTransmissionOpportunities(config, stream, release_time, 110 + constant::propagation_delay);
    ASSERT_FALSE(reservations.empty());
    EXPECT_EQ(reservations.begin()->start_time, 100);
    EXPECT_EQ(reservations.begin()->next_slot_start, 110);

    // deadline during slot, transmission does not fit anymore
    release_time = 100;
    reservations = utilizationList.searchTransmissionOpportunities(config, stream, release_time, 105);
    EXPECT_TRUE(reservations.empty());
}

TEST(UtilizationListSearchTransmissionOpportunities, case_deadline_at_slot_end)
{
    constexpr std::size_t hyper_cycle = 2000;
    auto utilizationList = common::NetworkUtilizationList{1, hyper_cycle, 100};

    // block [0, 99] and [120, hyper_cycle]
    utilizationList.reserveSlot(common::SlotReservationRequest{.egress_queue = common::NetworkQueueID{0},
                                                               .start_time = 0,
                                                               .next_slot_start = 100,
                                                               .arrival_time = 0},
                                common::FlowNodeID{1}, common::ConfigurationNodeID{1});
    utilizationList.reserveSlot(common::SlotReservationRequest{.egress_queue = common::NetworkQueueID{0},
                                                               .start_time = 120,
                                                               .next_slot_start = hyper_cycle,
                                                               .arrival_time = 120},
                                common::FlowNodeID{1}, common::ConfigurationNodeID{1});

    constexpr auto deadline = 120;
    const auto stream = graph_structs::Flow{.id = common::FlowNodeID{2}, .frame_size = 1250, .period = 1000, .source = common::NetworkNodeID{0}, .destination = common::NetworkNodeID{1}};
    const auto config = graph_structs::Configuration{.id = common::ConfigurationNodeID{2}, .flow = stream.id, .path = {common::NetworkQueueID{0}}};
    // Deadline at slot end, transmission fits before
    auto release_time = 110 - constant::propagation_delay;
    auto reservations = utilizationList.searchTransmissionOpportunities(config, stream, release_time, deadline);
    EXPECT_FALSE(reservations.empty());
    EXPECT_EQ(reservations.begin()->start_time, 110 - constant::propagation_delay);
    EXPECT_EQ(reservations.begin()->next_slot_start, 120 - constant::propagation_delay);

    // Deadline at slot end, transmission does not fit anymore
    release_time = 115;
    reservations = utilizationList.searchTransmissionOpportunities(config, stream, release_time, deadline);
    EXPECT_TRUE(reservations.empty());
}

TEST(UtilizationListSearchTransmissionOpportunities, case_perfect_placement)
{
    constexpr std::size_t hyper_cycle = 2000;
    auto utilizationList = common::NetworkUtilizationList{1, hyper_cycle, 100};

    // block [0, 110] and [120, hyper_cycle]
    utilizationList.reserveSlot(common::SlotReservationRequest{.egress_queue = common::NetworkQueueID{0},
                                                               .start_time = 0,
                                                               .next_slot_start = 110,
                                                               .arrival_time = 0},
                                common::FlowNodeID{1}, common::ConfigurationNodeID{1});
    utilizationList.reserveSlot(common::SlotReservationRequest{.egress_queue = common::NetworkQueueID{0},
                                                               .start_time = 120,
                                                               .next_slot_start = hyper_cycle,
                                                               .arrival_time = 120},
                                common::FlowNodeID{1}, common::ConfigurationNodeID{1});

    constexpr auto deadline = 200;
    const auto stream = graph_structs::Flow{.id = common::FlowNodeID{2}, .frame_size = 1250, .period = 1000, .source = common::NetworkNodeID{0}, .destination = common::NetworkNodeID{1}};
    const auto config = graph_structs::Configuration{.id = common::ConfigurationNodeID{2}, .flow = stream.id, .path = {common::NetworkQueueID{0}}};

    constexpr auto release_time = 0;
    const auto reservations = utilizationList.searchTransmissionOpportunities(config, stream, release_time, deadline);
    EXPECT_FALSE(reservations.empty());
    EXPECT_EQ(reservations.begin()->start_time, 110);
    EXPECT_EQ(reservations.begin()->next_slot_start, 120);
}


TEST(UtilizationListSearchTransmissionOpportunities, multipleHops)
{
    constexpr std::size_t hyper_cycle = 2000;
    constexpr std::size_t hops = 2;
    auto utilizationList = common::NetworkUtilizationList{hops, hyper_cycle, 100};
    std::vector<common::NetworkQueueID> path;
    path.reserve(hops);
    for(std::size_t i = 0; i < hops; ++i) {
        path.emplace_back(common::NetworkQueueID{i});
    }

    for(std::size_t i = 0; i < 2; ++i) {
        // frame sizes become smaller, so that the later frames catch up with the earlier ones
        const auto stream = graph_structs::Flow{.id = common::FlowNodeID{i}, .frame_size = 1000 / (i + 1), .period = hyper_cycle, .source = common::NetworkNodeID{0}, .destination = common::NetworkNodeID{1}};
        const auto config = graph_structs::Configuration{.id = common::ConfigurationNodeID{i}, .flow = stream.id, .path = path};

        // EXPECT_TRUE(utilizationList.searchTransmissionOpportunities(config, stream, 0, stream.period));
        // EXPECT_TRUE(utilizationList.searchTransmissionOpportunities(config, stream, stream.period, hyper_cycle));
        //  the placement calls the search again. We call the placement, so that we can check the reserved slots afterwards
        EXPECT_TRUE(placement::placeConfigASAP(config, stream, utilizationList));

        check_reservation_overlaps(utilizationList);
    }
}
