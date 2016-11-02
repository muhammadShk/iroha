/*
Copyright Soramitsu Co., Ltd. 2016 All Rights Reserved.
http://soramitsu.co.jp

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "merkle_transaction_repository.hpp"
#include "../world_state_repository.hpp"

#include <memory>
#include <iostream>

#include "../../util/logger.hpp"
#include "../../crypto/hash.hpp"

namespace merkle_transaction_repository {

    //TODO: change bool to throw an exception instead
    bool commit(ConsensusEvent<T> &event) {
        std::vector<std::tuple<std::string, std::string>> batchCommit
          = {
                std::tuple<std::string, std::string>("last_insertion", event->tx->getHash()),
                std::tuple<std::string, std::string>(event->tx->getHash(), event->tx->getAsText())
        };

        calculateNewRootHash(event, batchCommit);

        return repository::world_state_repository::addBatch<std::string>(batchCommit);
    }

    bool leafExists(const std::string& hash) {
        return !repository::world_state_repository::find(hash).empty();
    }

    std::string getLeaf(const std::string& hash) {
        return repository::world_state_repository::find(hash);
    }

    std::string calculateNewRootHash(const std::unique_ptr<consensus_event::ConsensusEvent> &event,
                                     std::vector<std::tuple<std::string, std::string>> &batchCommit) {

        std::unique_ptr<std::string> lastInsertion = repository::world_state_repository::find("last_insertion");

        if (lastInsertionHash.empty()) {
            // Note: there is no need to update the tree's DB here, because there is only one transaction--the current!
            return event->tx->getHash();
        }

        std::unique_ptr<std::string> parent = repository::world_state_repository::find(lastInsertion + "_parent");
        std::unique_ptr<std::string> leftChild = repository::world_state_repository::find(parent + "_leftChild");
        std::unique_ptr<std::string> rightChild = repository::world_state_repository::find(parent + "_rightChild");

        if (rightChild.empty()) {
            // insert the event's transaction as the right child
            rightChild = event->tx->getHash();
            std::string newParentHash = hash::sha3_256_hex(leftChild + rightChild);

            if (!batchCommit.empty()) { // TODO: this may not be the best comparison to use
                batchCommit.push_back(std::tuple<std::string, std::string>(lastInsertion + "_rightChild", rightChild));
                batchCommit.push_back(std::tuple<std::string, std::string>(lastInsertion + "_parent", rightChild));
            }

            // Propagate up the tree to the root
            while (!parent.empty()) {
                std::string newParentHash = hash::sha3_256_hex(leftChild + rightChild);

                if (!batchCommit.empty()) { // TODO: this may not be the best comparison to use
                    batchCommit.push_back(std::tuple<std::string, std::string>(newParentHash + "_parent", rightChild));
                    // TODO: delete old, unused nodes
                }

                // increment the right child and the parent, to move up the tree
                rightChild = newParentHash;
                parent = repository::world_state_repository::find(parent + "_parent");
            }

            if (!batchCommit.empty()) { // TODO: this may not be the best comparison to use
                batchCommit.push_back(std::tuple<std::string, std::string>("merkle_root", rightChild));
                // TODO: delete old, unused nodes
            }
            return rightChild;

        } else {
            std::string currHash= event->tx->getHash();

            // create a new node and put it on the left
            currHash = currNode.hash;
            currNode = hash::sha3_256_hex(currHash + event->tx->getHash());

            newparent = hash::sha3_256_hex(parent + event->tx->getHash());

            if (!batchCommit.empty()) { // TODO: this may not be the best comparison to use
                batchCommit.push_back(std::tuple<std::string, std::string>(event->tx->getHash() + "_parent",
                                                                           leftChild));
                batchCommit.push_back(std::tuple<std::string, std::string>(event->tx->getHash() + "_leftChild",
                                                                           leftChild));
            }

            // Propagate up the tree to the root
            while (!parent.empty()) {
                // find insertion point for new node
                currHash = currNode.hash;
                currNode = std::make_unique<MerkleNode>(currNode.parent);
            }
        }
    }
};  // namespace merkle_transaction_repository
