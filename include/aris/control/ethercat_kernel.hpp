﻿#ifndef ARIS_CONTROL_ETHERCAT_KERNEL_H_
#define ARIS_CONTROL_ETHERCAT_KERNEL_H_

#include <cstdint>
#include <any>
#include "ethercat.hpp"

namespace aris::control
{
	//------------------------ Ecrt 的配置流程 ------------------------//
	// 1. init master
	// 2. init slave
	// 3. init pdo
	// 4. init pdo_entry
	// 5. config pdo_entry
	// 6. config pdo
	// 7. config slave
	// 8. config master
	// 9. config sdo
	// 10.lock memory
	// 11.start master
	// 12.start slave

	//------------------------ Ecrt 的通讯流程 ------------------------//
	// 1. master receive
	// 2. slave receive
	// 3. pdo read update
	// 4. control strategy
	// 5. pdo write update
	// 6. master sync
	// 7. slave send
	// 8. master send

	// pdo可以分为3级 //
	// 1. Sm(sync manager)  :   每个sm可以同步多个pdo
	// 2. pdo               :   每个pdo 有index，此外还包含多个pdo entry
	// 3. pdo entry         :   每个pdo entry 对应一个 index 和 subindex

	class EthercatMaster;
	class PdoEntry;

	auto aris_ecrt_scan(EthercatMaster *master)->int;

	auto aris_ecrt_master_request(EthercatMaster *master)->void;
	auto aris_ecrt_master_stop(EthercatMaster *master)->void;
	auto aris_ecrt_master_recv(EthercatMaster *master)->void;
	auto aris_ecrt_master_send(EthercatMaster *master)->void;

	auto aris_ecrt_master_link_state(EthercatMaster* mst, EthercatMaster::MasterLinkState *master_state, EthercatMaster::SlaveLinkState *slave_state)->void;

	auto aris_ecrt_pdo_read(const PdoEntry *entry, void *data)->void;
	auto aris_ecrt_pdo_write(PdoEntry *entry, const void *data)->void;
	auto aris_ecrt_sdo_read(std::any& master, std::uint16_t slave_position, std::uint16_t index, std::uint8_t subindex,
		std::uint8_t *to_buffer, std::size_t byte_size, std::size_t *result_size, std::uint32_t *abort_code)->int;
	auto aris_ecrt_sdo_write(std::any& master, std::uint16_t slave_position, std::uint16_t index, std::uint8_t subindex,
		std::uint8_t *to_buffer, std::size_t byte_size, std::uint32_t *abort_code) ->int;
	auto aris_ecrt_sdo_config(std::any& master, std::any& slave, std::uint16_t index, std::uint8_t subindex,
		std::uint8_t *buffer, std::size_t byte_size)->void;
}

#endif
