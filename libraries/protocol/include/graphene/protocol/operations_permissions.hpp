#pragma once
#include <graphene/protocol/operations.hpp>

namespace graphene { namespace chain {
   class transaction_evaluation_state;
}}

namespace graphene { namespace protocol {

   using graphene::chain::transaction_evaluation_state;

   template<typename T> struct transform_to_operations_permissions;
   template<typename ...T>
   struct transform_to_operations_permissions<fc::static_variant<T...>>
   {
      typedef fc::static_variant< typename T::operation_permissions_type... > type;
   };
   typedef transform_to_operations_permissions<operation>::type operations_permissions_parameters;
	
	template<typename Operation>
   class permissions_helper {
     public:
      const typename Operation::operation_permissions_type& cget(const flat_set<operations_permissions_parameters>& parameters)const
      {
         auto itr = parameters.find( typename Operation::operation_permissions_type() );
         FC_ASSERT( itr != parameters.end() );
         return itr->template get<typename Operation::operation_permissions_type>();
      }
   };

   /**
    *  @brief contains all of the parameters necessary to verify the permission for any operation
    */
   struct operations_permissions
   {
      operations_permissions() = default;
      bool is_allowed(transaction_evaluation_state* trx_state, const operation& op) const;

      static operations_permissions get_default();

      void enable_all_operations();

      /**
       *  Validates all of the parameters are present and accounted for.
       */
      void validate()const;

      template<typename Operation>
         const typename Operation::operation_permissions_type& get()const
      {
         return permissions_helper<Operation>().cget(parameters);
      }
      template<typename Operation>
         typename Operation::operation_permissions_type& get()
      {
         return permissions_helper<Operation>().get(parameters);
      }

      /**
       *  @note must be sorted by operations_permissions_parameters.which() and have no duplicates
       */
      flat_set<operations_permissions_parameters> parameters;
   };

   typedef operations_permissions operations_permissions_type;

} } // graphene::protocol

FC_REFLECT_TYPENAME( graphene::protocol::operations_permissions_parameters )
FC_REFLECT( graphene::protocol::operations_permissions, (parameters) )

GRAPHENE_DECLARE_EXTERNAL_SERIALIZATION( graphene::protocol::operations_permissions )