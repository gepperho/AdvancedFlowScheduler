#include <IO/InputParser.h>


#include <gtest/gtest.h>

TEST(UtilizationListTest, init)
{
    constexpr auto hyper_cycle = std::size_t{100};
    constexpr auto number_of_network_links = std::size_t{10};
    const common::NetworkUtilizationList utilization_list{number_of_network_links, hyper_cycle, 100};

    ASSERT_EQ(utilization_list.getFreeEgressSlots().size(), number_of_network_links);
    for(const auto& free_list : utilization_list.getFreeEgressSlots()) {
        ASSERT_EQ(free_list.size(), 1);
        const auto first_iter = free_list.begin();
        ASSERT_EQ(first_iter->start_time, 0);
        ASSERT_EQ(first_iter->last_free_macro_tick, hyper_cycle - 1);
    }

    ASSERT_EQ(utilization_list.getReservedEgressSlots().size(), number_of_network_links);
    for(const auto& reserved_list : utilization_list.getReservedEgressSlots()) {
        ASSERT_EQ(reserved_list.size(), 0);
    }
}

TEST(UtilizationListTest, insertSlots)
{
    // Flow 0
    graph_structs::Flow f0{.id = common::FlowNodeID{0}, .frame_size = 125, .period = 15};
    graph_structs::Configuration c0{.id = common::ConfigurationNodeID{0}, .flow = f0.id, .path = std::vector{common::NetworkQueueID{0}}};
    // Flow 1
    graph_structs::Flow f1{.id = common::FlowNodeID{1}, .frame_size = 375, .period = 15};
    graph_structs::Configuration c1{.id = common::ConfigurationNodeID{1}, .flow = f1.id, .path = std::vector{common::NetworkQueueID{0}}};
    // Flow 2
    graph_structs::Flow f2{.id = common::FlowNodeID{2}, .frame_size = 125, .period = 15};
    graph_structs::Configuration c2{.id = common::ConfigurationNodeID{2}, .flow = f2.id, .path = std::vector{common::NetworkQueueID{0}}};

    common::NetworkUtilizationList utilization_list{2, 15, 15};
    auto success = placement::placeConfigASAP(c0, f0, utilization_list);
    success = std::min(placement::placeConfigASAP(c1, f1, utilization_list), success);
    success = std::min(placement::placeConfigASAP(c2, f2, utilization_list), success);

    ASSERT_TRUE(success);

    // make sure the slots are reserved correctly
    const auto& reserved = utilization_list.getReservedEgressSlots();
    auto reserved_it = reserved[0].begin();
    ASSERT_EQ(reserved_it->flow_id.get(), f0.id.get());
    reserved_it = std::next(reserved_it, 1);
    ASSERT_EQ(reserved_it->flow_id.get(), f1.id.get());
    reserved_it = std::next(reserved_it, 1);
    ASSERT_EQ(reserved_it->flow_id.get(), f2.id.get());

    // make sure there is only one large free slot left at the end
    const auto& free = utilization_list.getFreeEgressSlots();
    ASSERT_EQ(free[0].size(), 1);
    auto free_it = free[0].begin();
    ASSERT_EQ(free_it->start_time, 5);
    ASSERT_EQ(free_it->last_free_macro_tick, 14);

    // remove Flow 1 and check if the slot is released correctly
    std::vector flows_to_be_removed = {f1.id};
    utilization_list.removeConfigs(flows_to_be_removed);

    // make sure there is a new free slot between f0 and f2, while the large free slot still exits
    ASSERT_EQ(free[0].size(), 2);
    free_it = utilization_list.getFreeEgressSlots()[0].begin();
    ASSERT_EQ(free_it->start_time, 1);
    ASSERT_EQ(free_it->last_free_macro_tick, 3);
    free_it = std::next(free_it, 1);
    ASSERT_EQ(free_it->start_time, 5);
    ASSERT_EQ(free_it->last_free_macro_tick, 14);

    // make sure the reserved slot is removed correctly, e.g. the ones still remaining are from f0 and f2
    ASSERT_EQ(utilization_list.getReservedEgressSlots()[0].size(), 2);
    reserved_it = utilization_list.getReservedEgressSlots()[0].begin();
    ASSERT_EQ(reserved_it->flow_id.get(), f0.id.get());
    reserved_it = std::next(reserved_it, 1);
    ASSERT_EQ(reserved_it->flow_id.get(), f2.id.get());

    // now test a perfect insert
    // Flow 3, equals the removed Flow 1
    graph_structs::Flow f3{.id = common::FlowNodeID{3}, .frame_size = 375, .period = 15};
    graph_structs::Configuration c3{.id = common::ConfigurationNodeID{3}, .flow = f3.id, .path = std::vector{common::NetworkQueueID{0}}};
    success = placement::placeConfigASAP(c3, f3, utilization_list);
    ASSERT_TRUE(success);
    ASSERT_EQ(utilization_list.getFreeEgressSlots()[0].size(), 1);
}

TEST(UtilizationListTest, reserveSlotsPartialMatchAtSlotEnd)
{
    constexpr std::size_t hyper_cycle = 20;
    // transmission time of 1, f0-f2 have 5
    const graph_structs::Flow f3_pre{.id = common::FlowNodeID{100}, .frame_size = 125, .period = hyper_cycle};
    const graph_structs::Configuration c3_pre{.id = common::ConfigurationNodeID{100}, .flow = f3_pre.id, .path = std::vector{common::NetworkQueueID{0}}};

    const graph_structs::Flow f0{.id = common::FlowNodeID{0}, .frame_size = 625, .period = hyper_cycle};
    const graph_structs::Configuration c0{.id = common::ConfigurationNodeID{0}, .flow = f0.id, .path = std::vector{common::NetworkQueueID{0}}};

    const graph_structs::Flow f1{.id = common::FlowNodeID{1}, .frame_size = 625, .period = hyper_cycle};
    const graph_structs::Configuration c1{.id = common::ConfigurationNodeID{0}, .flow = f1.id, .path = std::vector{common::NetworkQueueID{0}}};

    const graph_structs::Flow f2{.id = common::FlowNodeID{2}, .frame_size = 625, .period = hyper_cycle};
    const graph_structs::Configuration c2{.id = common::ConfigurationNodeID{0}, .flow = f2.id, .path = std::vector{common::NetworkQueueID{0}}};


    common::NetworkUtilizationList utilization_list{2, hyper_cycle, hyper_cycle};
    const auto& free_slots = utilization_list.getFreeEgressSlots().at(0);


    auto success = placement::placeConfigASAP(c0, f3_pre, utilization_list);
    success = std::min(placement::placeConfigASAP(c1, f0, utilization_list), success);
    success = std::min(placement::placeConfigASAP(c1, f1, utilization_list), success);
    success = std::min(placement::placeConfigASAP(c2, f2, utilization_list), success);
    ASSERT_TRUE(success);
    EXPECT_EQ(free_slots.size(), 1);

    // remove the center flow
    std::vector flows_to_be_removed = {f1.id, f3_pre.id};
    utilization_list.removeConfigs(flows_to_be_removed);
    ASSERT_EQ(free_slots.size(), 3);
    EXPECT_EQ(free_slots.front().start_time, 0);
    EXPECT_EQ(free_slots.front().last_free_macro_tick, 0);
    EXPECT_EQ(free_slots.at(1).start_time, 6);
    EXPECT_EQ(free_slots.at(1).last_free_macro_tick, 10);

    const graph_structs::Flow f3{.id = common::FlowNodeID{3}, .frame_size = 125, .period = hyper_cycle / 2};
    const graph_structs::Configuration c3{.id = common::ConfigurationNodeID{3}, .flow = f3.id, .path = std::vector{common::NetworkQueueID{0}}};

    ASSERT_TRUE(placement::placeConfigASAP(c3, f3, utilization_list));
    ASSERT_EQ(free_slots.size(), 2);
    EXPECT_EQ(free_slots.front().start_time, 6);
    EXPECT_EQ(free_slots.front().last_free_macro_tick, 9);
}

TEST(UtilizationListTestCornerCases, cornerCases1)
{
    /* A free slot runs longer than the effective deadline, so that the frame fits in the slot, but would not be completely transmitted
     * before the effective deadline is reached.
     */

    constexpr std::size_t hyper_cycle = 200;
    constexpr std::size_t sub_cycle = hyper_cycle / 4;
    constexpr std::size_t period = hyper_cycle / 2;

    common::NetworkUtilizationList utilization_list{2, hyper_cycle, sub_cycle};
    const auto& free_slots = utilization_list.getFreeEgressSlots().at(0);

    // block some time at the very beginning
    utilization_list.reserveSlot(common::SlotReservationRequest{.egress_queue = common::NetworkQueueID{0}, .start_time = 0, .next_slot_start = 20, .arrival_time = 0},
                                 common::FlowNodeID{0}, common::ConfigurationNodeID{0});

    // block some time in the second sub-cycle on the second link, so that f arrives (too) late at the second hop
    utilization_list.reserveSlot(common::SlotReservationRequest{.egress_queue = common::NetworkQueueID{0}, .start_time = 55, .next_slot_start = 80, .arrival_time = 0},
                                 common::FlowNodeID{0}, common::ConfigurationNodeID{0});

    const graph_structs::Flow f{.id = common::FlowNodeID{100}, .frame_size = 1250, .period = period};
    const graph_structs::Configuration c{.id = common::ConfigurationNodeID{100}, .flow = f.id, .path = std::vector{common::NetworkQueueID{0}, common::NetworkQueueID{1}}};

    // the second sub-cycle is blocked
    const auto sub_cylce_2_transmission_opportunities = utilization_list.searchTransmissionOpportunities(c, f, 50, 100);
    EXPECT_TRUE(sub_cylce_2_transmission_opportunities.empty());

    // f should be placed in the first sub cycle
    ASSERT_TRUE(placement::placeConfigBalanced(c, f, utilization_list));
    // f was placed in the first sub cycle
    EXPECT_EQ(free_slots.front().start_time, 30);
}

TEST(UtilizationListTestCornerCases, cornerCases2)
{
    /* There is a free slot available at the end of sub-cycle two, so that the frame arrives just in time.
     * Sub-cycle 1 is superior.
     */

    constexpr std::size_t hyper_cycle = 200;
    constexpr std::size_t sub_cycle = hyper_cycle / 4;
    constexpr std::size_t period = hyper_cycle / 2;

    common::NetworkUtilizationList utilization_list{2, hyper_cycle, sub_cycle};
    const auto& free_slots = utilization_list.getFreeEgressSlots().at(0);

    // block some time at the very beginning
    utilization_list.reserveSlot(common::SlotReservationRequest{.egress_queue = common::NetworkQueueID{0}, .start_time = 0, .next_slot_start = 20, .arrival_time = 0},
                                 common::FlowNodeID{0}, common::ConfigurationNodeID{0});

    // block some time in the second sub-cycle on the second link, so that f arrives later than in the first sub-cycle
    utilization_list.reserveSlot(common::SlotReservationRequest{.egress_queue = common::NetworkQueueID{0}, .start_time = 55, .next_slot_start = 74, .arrival_time = 0},
                                 common::FlowNodeID{0}, common::ConfigurationNodeID{0});

    const graph_structs::Flow f{.id = common::FlowNodeID{100}, .frame_size = 1250, .period = period};
    const graph_structs::Configuration c{.id = common::ConfigurationNodeID{100}, .flow = f.id, .path = std::vector{common::NetworkQueueID{0}, common::NetworkQueueID{1}}};

    // the second sub-cycle is blocked
    const auto sub_cylce_2_transmission_opportunities = utilization_list.searchTransmissionOpportunities(c, f, 50, 100);
    EXPECT_FALSE(sub_cylce_2_transmission_opportunities.empty());
    EXPECT_EQ(sub_cylce_2_transmission_opportunities.at(1).next_slot_start, period - constant::propagation_delay);

    // f should be placed in the first sub cycle
    ASSERT_TRUE(placement::placeConfigBalanced(c, f, utilization_list));
    // f was placed in the first sub cycle
    EXPECT_EQ(free_slots.front().start_time, 30);
}



TEST(UtilizationListTest, freePlacement)
{
    constexpr std::size_t hyper_cycle = 15;
    // Flow 0, will be placed "free" in queue 1
    const graph_structs::Flow f0{.id = common::FlowNodeID{0}, .frame_size = 250, .period = hyper_cycle};
    const std::vector path = {common::NetworkQueueID{0}, common::NetworkQueueID{1}};
    const graph_structs::Configuration c0{.id = common::ConfigurationNodeID{0}, .flow = f0.id, .path = path};

    common::NetworkUtilizationList utilization_list{4, hyper_cycle, hyper_cycle};
    const auto success = placement::placeConfigASAP(c0, f0, utilization_list);
    ASSERT_TRUE(success);

    const auto transmission_time = util::calculate_transmission_delay(f0.frame_size);

    auto& egress_queue_1_free_slots = utilization_list.getFreeEgressSlots()[1];
    ASSERT_EQ(egress_queue_1_free_slots.size(), 2);

    ASSERT_EQ(egress_queue_1_free_slots[0].start_time, 0);
    ASSERT_EQ(egress_queue_1_free_slots[0].last_free_macro_tick, transmission_time + constant::propagation_delay + constant::processing_delay - 1);

    ASSERT_EQ(egress_queue_1_free_slots[1].start_time, (2 * transmission_time) + constant::propagation_delay + constant::processing_delay);
    ASSERT_EQ(egress_queue_1_free_slots[1].last_free_macro_tick, hyper_cycle - 1);
}

TEST(UtilizationListTest, freeSlots)
{
    // Flow 0
    graph_structs::Flow f0{.id = common::FlowNodeID{0}, .frame_size = 125, .period = 15};
    graph_structs::Configuration c0{.id = common::ConfigurationNodeID{0}, .flow = f0.id, .path = std::vector{common::NetworkQueueID{0}}};
    // Flow 1
    graph_structs::Flow f1{.id = common::FlowNodeID{1}, .frame_size = 375, .period = 15};
    graph_structs::Configuration c1{.id = common::ConfigurationNodeID{1}, .flow = f1.id, .path = std::vector{common::NetworkQueueID{0}}};
    // Flow 2
    graph_structs::Flow f2{.id = common::FlowNodeID{2}, .frame_size = 125, .period = 15};
    graph_structs::Configuration c2{.id = common::ConfigurationNodeID{2}, .flow = f2.id, .path = std::vector{common::NetworkQueueID{0}}};
    // Flow 3
    graph_structs::Flow f3{.id = common::FlowNodeID{3}, .frame_size = 125, .period = 45};
    graph_structs::Configuration c3{.id = common::ConfigurationNodeID{3}, .flow = f3.id, .path = std::vector{common::NetworkQueueID{0}}};

    common::NetworkUtilizationList utilization_list{2, 45, 15};
    auto success = placement::placeConfigASAP(c0, f0, utilization_list);
    success = std::min(placement::placeConfigASAP(c1, f1, utilization_list), success);
    success = std::min(placement::placeConfigASAP(c2, f2, utilization_list), success);
    success = std::min(placement::placeConfigASAP(c3, f3, utilization_list), success);

    ASSERT_TRUE(success);

    // make sure the slots are reserved correctly
    const auto& reserved = utilization_list.getReservedEgressSlots();
    std::vector expected_slot_reservations{
        std::tuple(0, 1, f0.id),
        std::tuple(15, 16, f0.id),
        std::tuple(30, 31, f0.id),
        std::tuple(1, 4, f1.id),
        std::tuple(16, 19, f1.id),
        std::tuple(31, 34, f1.id),
        std::tuple(4, 5, f2.id),
        std::tuple(19, 20, f2.id),
        std::tuple(34, 35, f2.id),
        std::tuple(5, 6, f3.id)};

    for(std::size_t index = 0; index < reserved.size(); ++index) {
        auto& [reserved_start, reserved_next_start, reservedFlowID, reservedConfigID] = reserved[0].at(index);
        auto& [expected_start, expected_next_start, expectedFlowID] = expected_slot_reservations.at(index);
        ASSERT_EQ(reservedConfigID.get(), expectedFlowID.get());
        ASSERT_EQ(reserved_start, expected_start);
        ASSERT_EQ(reserved_next_start, expected_next_start);
    }

    // make sure there are empty slots
    const auto& free = utilization_list.getFreeEgressSlots();
    ASSERT_EQ(free[0].size(), 3);


    // remove Flow 1 and check if the slot is released correctly
    std::vector flows_to_be_removed = {f1.id};
    utilization_list.removeConfigs(flows_to_be_removed);

    // make sure there are new free slots between f0 and f2, while the large free slots still exit
    ASSERT_EQ(free[0].size(), 6);
    std::vector<std::pair<std::size_t, std::size_t>> expected_free_slots{
        std::pair(1, 3),
        std::pair(6, 14),
        std::pair(16, 18),
        std::pair(20, 29),
        std::pair(31, 33),
        std::pair(35, 44),
    };
    for(std::size_t index = 0; index < free[0].size(); ++index) {
        // FreeSlot: <start_time, last_free_macro_tick>
        auto& [free_slot_first, free_slot_last] = free[0].at(index);
        ASSERT_EQ(free_slot_first, expected_free_slots.at(index).first);
        ASSERT_EQ(free_slot_last, expected_free_slots.at(index).second);
    }

    // make sure the reserved slots are removed
    ASSERT_EQ(utilization_list.getReservedEgressSlots()[0].size(), 7);

    // now test a perfect insert
    // Flow 4, equals the removed Flow 1
    graph_structs::Flow f4{.id = common::FlowNodeID{4}, .frame_size = 375, .period = 15};
    graph_structs::Configuration c4{.id = common::ConfigurationNodeID{4}, .flow = f4.id, .path = std::vector{common::NetworkQueueID{0}}};
    success = placement::placeConfigASAP(c4, f4, utilization_list);
    ASSERT_TRUE(success);
    ASSERT_EQ(utilization_list.getFreeEgressSlots()[0].size(), 3);
}

TEST(UtilizationListTest, freeSlotsCornerCaseFirstSlot)
{
    common::NetworkUtilizationList utilization_list{2, 40, 20};

    for(std::size_t index = 0; index < 7; ++index) {
        graph_structs::Flow f{.id = common::FlowNodeID{index}, .frame_size = 250, .period = 20};
        graph_structs::Configuration c{.id = common::ConfigurationNodeID{index}, .flow = f.id, .path = std::vector{common::NetworkQueueID{0}}};
        bool success = placement::placeConfigASAP(c, f, utilization_list);
        ASSERT_TRUE(success);
    }

    const auto& reserved = utilization_list.getReservedEgressSlots()[0];
    const auto& free = utilization_list.getFreeEgressSlots()[0];


    utilization_list.removeConfigs({common::FlowNodeID{2}});
    ASSERT_EQ(free.size(), 4);
    ASSERT_EQ(reserved.size(), 12);

    // inner corner case
    utilization_list.removeConfigs({common::FlowNodeID{3}});
    ASSERT_EQ(free.size(), 4);
    ASSERT_EQ(reserved.size(), 10);

    std::vector<std::pair<std::size_t, std::size_t>> expected_free_slots = {
        std::pair(4, 7),
        std::pair(14, 19),
        std::pair(24, 27),
        std::pair(34, 39)};
    for(std::size_t index = 0; index < free.size(); ++index) {
        auto& [free_slot_first, free_slot_last] = free.at(index);
        auto [expected_slot_first, expected_slot_last] = expected_free_slots.at(index);
        ASSERT_EQ(free_slot_first, expected_slot_first);
        ASSERT_EQ(free_slot_last, expected_slot_last);
    }

    // outer corner case
    utilization_list.removeConfigs({common::FlowNodeID{1}});
    ASSERT_EQ(free.size(), 4);
    ASSERT_EQ(reserved.size(), 8);

    expected_free_slots = {
        std::pair(2, 7),
        std::pair(14, 19),
        std::pair(22, 27),
        std::pair(34, 39)};
    for(std::size_t index = 0; index < free.size(); ++index) {
        auto [free_slot_first, free_slot_last] = free.at(index);
        auto [expected_slot_first, expected_slot_last] = expected_free_slots.at(index);
        ASSERT_EQ(free_slot_first, expected_slot_first);
        ASSERT_EQ(free_slot_last, expected_slot_last);
    }
}


TEST(UtilizationListTest, freeSlotsCornerCaseLastFreeSlot)
{
    common::NetworkUtilizationList utilization_list{2, 40, 20};
    // fill up utilization list
    for(std::size_t index = 0; index < 9; ++index) {
        graph_structs::Flow f{.id = common::FlowNodeID{index}, .frame_size = 250, .period = 20};
        graph_structs::Configuration c{.id = common::ConfigurationNodeID{index}, .flow = f.id, .path = std::vector{common::NetworkQueueID{0}}};
        bool success = placement::placeConfigASAP(c, f, utilization_list);
        ASSERT_TRUE(success);
    }

    // add last flow manually because it wouldn't be schedulable due to the existence of a propagation delay
    graph_structs::Flow f{.id = common::FlowNodeID{9}, .frame_size = 250, .period = 20};
    graph_structs::Configuration c{.id = common::ConfigurationNodeID{9}, .flow = f.id, .path = std::vector{common::NetworkQueueID{0}}};
    utilization_list.reserveSlot(common::SlotReservationRequest{common::NetworkQueueID{0}, 18, 20, 0}, f.id, c.id);
    utilization_list.reserveSlot(common::SlotReservationRequest{common::NetworkQueueID{0}, 38, 40, 20}, f.id, c.id);


    const auto& reserved = utilization_list.getReservedEgressSlots()[0];
    const auto& free = utilization_list.getFreeEgressSlots()[0];
    ASSERT_EQ(free.size(), 0);
    ASSERT_EQ(reserved.size(), 20);

    // test corner case: adjacent to last free slot
    utilization_list.removeConfigs({common::FlowNodeID{6}});
    ASSERT_EQ(free.size(), 2);
    ASSERT_EQ(reserved.size(), 18);
    std::vector<std::pair<std::size_t, std::size_t>> expected_free_slots = {
        std::pair(12, 13),
        std::pair(32, 33)};
    for(std::size_t index = 0; index < free.size(); ++index) {
        auto [free_slot_first, free_slot_last] = free.at(index);
        auto [expected_slot_first, expected_slot_last] = expected_free_slots.at(index);
        ASSERT_EQ(free_slot_first, expected_slot_first);
        ASSERT_EQ(free_slot_last, expected_slot_last);
    }
    // inner corner case
    utilization_list.removeConfigs({common::FlowNodeID{5}});
    ASSERT_EQ(free.size(), 2);
    ASSERT_EQ(reserved.size(), 16);
    expected_free_slots = {
        std::pair(10, 13),
        std::pair(30, 33)};
    for(std::size_t index = 0; index < free.size(); ++index) {
        auto [free_slot_first, free_slot_last] = free.at(index);
        auto [expected_slot_first, expected_slot_last] = expected_free_slots.at(index);
        ASSERT_EQ(free_slot_first, expected_slot_first);
        ASSERT_EQ(free_slot_last, expected_slot_last);
    }

    // actual corner case
    utilization_list.removeConfigs({common::FlowNodeID{7}});
    ASSERT_EQ(free.size(), 2);
    ASSERT_EQ(reserved.size(), 14);
    expected_free_slots = {
        std::pair(10, 15),
        std::pair(30, 35)};
    for(std::size_t index = 0; index < free.size(); ++index) {
        auto [free_slot_first, free_slot_last] = free.at(index);
        auto [expected_slot_first, expected_slot_last] = expected_free_slots.at(index);
        ASSERT_EQ(free_slot_first, expected_slot_first);
        ASSERT_EQ(free_slot_last, expected_slot_last);
    }
}

TEST(UtilizationListTest, freeSlotsMiddleCase)
{
    /**
     * here two slots are freed with distance 1 (slot).
     * Then the slot in the middle is freed.
     */
    common::NetworkUtilizationList utilization_list{2, 40, 20};
    // fill up utilization list
    for(std::size_t index = 0; index < 9; ++index) {
        graph_structs::Flow f{.id = common::FlowNodeID{index}, .frame_size = 250, .period = 20};
        graph_structs::Configuration c{.id = common::ConfigurationNodeID{index}, .flow = f.id, .path = std::vector{common::NetworkQueueID{0}}};
        bool success = placement::placeConfigASAP(c, f, utilization_list);
        ASSERT_TRUE(success);
    }


    const auto& reserved = utilization_list.getReservedEgressSlots()[0];
    const auto& free = utilization_list.getFreeEgressSlots()[0];
    ASSERT_EQ(free.size(), 2);
    ASSERT_EQ(reserved.size(), 18);

    // test corner case: adjacent to last free slot
    utilization_list.removeConfigs({common::FlowNodeID{3}});
    ASSERT_EQ(free.size(), 4);
    ASSERT_EQ(reserved.size(), 16);
    std::vector<std::pair<std::size_t, std::size_t>> expected_free_slots = {
        std::pair(6, 7),
        std::pair(18, 19),
        std::pair(26, 27),
        std::pair(38, 39)};
    for(std::size_t index = 0; index < free.size(); ++index) {
        auto [free_slot_first, free_slot_last] = free.at(index);
        auto [expected_slot_first, expected_slot_last] = expected_free_slots.at(index);
        ASSERT_EQ(free_slot_first, expected_slot_first);
        ASSERT_EQ(free_slot_last, expected_slot_last);
    }
    // inner corner case
    utilization_list.removeConfigs({common::FlowNodeID{5}});
    ASSERT_EQ(free.size(), 6);
    ASSERT_EQ(reserved.size(), 14);
    expected_free_slots = {
        std::pair(6, 7),
        std::pair(10, 11),
        std::pair(18, 19),
        std::pair(26, 27),
        std::pair(30, 31),
        std::pair(38, 39)};
    for(std::size_t index = 0; index < free.size(); ++index) {
        auto [free_slot_first, free_slot_last] = free.at(index);
        auto [expected_slot_first, expected_slot_last] = expected_free_slots.at(index);
        ASSERT_EQ(free_slot_first, expected_slot_first);
        ASSERT_EQ(free_slot_last, expected_slot_last);
    }

    // actual corner case
    utilization_list.removeConfigs({common::FlowNodeID{4}});
    ASSERT_EQ(free.size(), 4);
    ASSERT_EQ(reserved.size(), 12);
    expected_free_slots = {
        std::pair(6, 11),
        std::pair(18, 19),
        std::pair(26, 31),
        std::pair(38, 39)};
    for(std::size_t index = 0; index < free.size(); ++index) {
        auto [free_slot_first, free_slot_last] = free.at(index);
        auto [expected_slot_first, expected_slot_last] = expected_free_slots.at(index);
        ASSERT_EQ(free_slot_first, expected_slot_first);
        ASSERT_EQ(free_slot_last, expected_slot_last);
    }
}

TEST(UtilizationListTest, freeMultipleSlots)
{
    common::NetworkUtilizationList utilization_list{2, 40, 20};

    for(std::size_t index = 0; index < 5; ++index) {
        graph_structs::Flow f{.id = common::FlowNodeID{index}, .frame_size = 250, .period = 20};
        graph_structs::Configuration c{.id = common::ConfigurationNodeID{index}, .flow = f.id, .path = std::vector{common::NetworkQueueID{0}}};
        bool success = placement::placeConfigASAP(c, f, utilization_list);
        ASSERT_TRUE(success);
    }

    const auto& reserved = utilization_list.getReservedEgressSlots()[0];
    const auto& free = utilization_list.getFreeEgressSlots()[0];

    ASSERT_EQ(reserved.size(), 10);
    ASSERT_EQ(free.size(), 2);

    utilization_list.removeConfigs({common::FlowNodeID{2}, common::FlowNodeID{1}});
    ASSERT_EQ(reserved.size(), 6);
    ASSERT_EQ(free.size(), 4);

    std::vector<std::pair<std::size_t, std::size_t>> expected_free_slots = {
        std::pair(2, 5),
        std::pair(10, 19),
        std::pair(22, 25),
        std::pair(30, 39)};

    // test corner case: first
    for(std::size_t index = 0; index < free.size(); ++index) {
        auto [free_slot_first, free_slot_last] = free.at(index);
        auto [expected_slot_first, expected_slot_last] = expected_free_slots.at(index);
        ASSERT_EQ(free_slot_first, expected_slot_first);
        ASSERT_EQ(free_slot_last, expected_slot_last);
    }
}

TEST(UtilizationListTest, balancedPlacement1)
{
    const std::vector path = {common::NetworkQueueID{0}, common::NetworkQueueID{1}};

    common::NetworkUtilizationList utilization_list{2, 800, 100};
    std::vector<graph_structs::Flow> flows = {
        {.id = common::FlowNodeID{0}, .frame_size = 250, .period = 200},
        {.id = common::FlowNodeID{1}, .frame_size = 250, .period = 400},
        {.id = common::FlowNodeID{2}, .frame_size = 250, .period = 400}};

    for(auto& flow : flows) {
        graph_structs::Configuration c{.id = common::ConfigurationNodeID{flow.id.get()}, .flow = flow.id, .path = path};
        auto success = placement::placeConfigBalanced(c, flow, utilization_list);
        ASSERT_TRUE(success);
    }

    auto& link1_slots = utilization_list.getReservedEgressSlots().at(0);
    EXPECT_EQ(8, link1_slots.size());
    const std::vector<std::size_t> expected_start_times = {0, 200, 400, 600, 100, 500, 300, 700};
    for(std::size_t index = 0; index < link1_slots.size(); ++index) {
        EXPECT_EQ(expected_start_times.at(index), link1_slots.at(index).start_time);
    }
}

TEST(UtilizationListTest, balancedPlacement2)
{
    /*
     * Place many streams at the hyper cycle start (using ASAP)
     * Then verify that adding a low period stream fails with the balanced placement.
     */
    const std::vector path = {common::NetworkQueueID{0}, common::NetworkQueueID{1}};

    common::NetworkUtilizationList utilization_list{2, 800, 100};
    std::vector<graph_structs::Flow> flows;
    for(std::size_t i = 0; i < 50; ++i) {
        flows.emplace_back(graph_structs::Flow{.id = common::FlowNodeID{i}, .frame_size = 250, .period = 200});
    }

    for(auto& flow : flows) {
        graph_structs::Configuration c{.id = common::ConfigurationNodeID{flow.id.get()}, .flow = flow.id, .path = path};
        auto success = placement::placeConfigASAP(c, flow, utilization_list);
        ASSERT_TRUE(success);
    }

    utilization_list.sortReservedEgressSlots();

    const auto& first_slot = utilization_list.getReservedEgressSlots().at(0).front();
    EXPECT_EQ(first_slot.start_time, 0);
    EXPECT_EQ(first_slot.next_slot_start, 2);
    const auto& last_slot_in_first_period = utilization_list.getReservedEgressSlots().at(0).at(49);
    EXPECT_EQ(last_slot_in_first_period.start_time, 98);
    EXPECT_EQ(last_slot_in_first_period.next_slot_start, 100);
    const auto& last_slot = utilization_list.getReservedEgressSlots().at(0).back();
    EXPECT_EQ(last_slot.start_time, 698);
    EXPECT_EQ(last_slot.next_slot_start, 700);

    graph_structs::Flow f{.id = common::FlowNodeID{50}, .frame_size = 125, .period = 100};
    graph_structs::Configuration c{.id = common::ConfigurationNodeID{f.id.get()}, .flow = f.id, .path = path};
    EXPECT_FALSE(placement::placeConfigBalanced(c, f, utilization_list));

    auto& link1_slots = utilization_list.getReservedEgressSlots().at(0);
    EXPECT_EQ(50 * 4, link1_slots.size());
}

TEST(UtilizationListTest, balancedPlacement3)
{
    /*
     * Comparable to 2, but use balanced all the way
     * -> more streams can be admitted.
     */
    std::vector path = {common::NetworkQueueID{0}, common::NetworkQueueID{1}};

    common::NetworkUtilizationList utilization_list{2, 800, 100};
    std::vector<graph_structs::Flow> flows;
    for(std::size_t i = 0; i < 50; ++i) {
        graph_structs::Flow f{.id = common::FlowNodeID{i}, .frame_size = 250, .period = 200};
        graph_structs::Configuration c{.id = common::ConfigurationNodeID{f.id.get()}, .flow = f.id, .path = path};
        auto success = placement::placeConfigBalanced(c, f, utilization_list);
        ASSERT_TRUE(success);
    }

    std::vector<graph_structs::Flow> flows2;
    for(std::size_t i = 0; i < 10; ++i) {
        graph_structs::Flow f{.id = common::FlowNodeID{50 + i}, .frame_size = 125, .period = 100};
        graph_structs::Configuration c{.id = common::ConfigurationNodeID{f.id.get()}, .flow = f.id, .path = path};
        bool success = placement::placeConfigBalanced(c, f, utilization_list);
        ASSERT_TRUE(success);
    }

    auto& link1_slots = utilization_list.getReservedEgressSlots().at(0);
    EXPECT_EQ(50 * 4 + 10 * 8, link1_slots.size());
}

TEST(UtilizationListTest, sortReserved)
{
    common::NetworkUtilizationList utilizationList{1, 1000, 100};
    // requiredSlot: network_segment_id, send_time, next_free_time, arrival_time
    std::vector<common::SlotReservationRequest> slots = {{common::NetworkQueueID{0}, 0, 10, 0},
                                                         {common::NetworkQueueID{0}, 900, 901, 900},
                                                         {common::NetworkQueueID{0}, 500, 550, 499},
                                                         {common::NetworkQueueID{0}, 20, 30, 11},
                                                         {common::NetworkQueueID{0}, 880, 899, 877},
                                                         {common::NetworkQueueID{0}, 10, 20, 5}};

    for(const auto& slot : slots) {
        constexpr std::size_t id = 0;
        utilizationList.reserveSlot(slot,
                                    common::FlowNodeID{id},
                                    common::ConfigurationNodeID{id * 10});
    }

    const auto& reserved = utilizationList.getReservedEgressSlots();
    // ensure the insert order is currently present
    for(std::size_t i = 0; i < reserved.size(); ++i) {
        ASSERT_EQ(reserved[0].at(i).start_time, slots.at(i).start_time);
        ASSERT_EQ(reserved[0].at(i).next_slot_start, slots.at(i).next_slot_start);
    }

    utilizationList.sortReservedEgressSlots();
    // ensure the sorting worked
    EXPECT_EQ(reserved[0].at(0).start_time, 0);
    EXPECT_EQ(reserved[0].at(0).next_slot_start, 10);
    EXPECT_EQ(reserved[0].at(1).start_time, 10);
    EXPECT_EQ(reserved[0].at(1).next_slot_start, 20);
    EXPECT_EQ(reserved[0].at(2).start_time, 20);
    EXPECT_EQ(reserved[0].at(2).next_slot_start, 30);
    EXPECT_EQ(reserved[0].at(3).start_time, 500);
    EXPECT_EQ(reserved[0].at(3).next_slot_start, 550);
    EXPECT_EQ(reserved[0].at(4).start_time, 880);
    EXPECT_EQ(reserved[0].at(4).next_slot_start, 899);
    EXPECT_EQ(reserved[0].at(5).start_time, 900);
    EXPECT_EQ(reserved[0].at(5).next_slot_start, 901);
}
