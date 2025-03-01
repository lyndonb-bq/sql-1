/*
 * Copyright OpenSearch Contributors
 * SPDX-License-Identifier: Apache-2.0
 */


package org.opensearch.sql.opensearch.storage.script.aggregation;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.containsInAnyOrder;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doAnswer;
import static org.opensearch.sql.data.type.ExprCoreType.DOUBLE;
import static org.opensearch.sql.data.type.ExprCoreType.INTEGER;
import static org.opensearch.sql.data.type.ExprCoreType.STRING;
import static org.opensearch.sql.expression.DSL.literal;
import static org.opensearch.sql.expression.DSL.named;
import static org.opensearch.sql.expression.DSL.ref;
import static org.opensearch.sql.expression.DSL.span;
import static org.opensearch.sql.opensearch.data.type.OpenSearchDataType.OPENSEARCH_TEXT_KEYWORD;
import static org.opensearch.sql.opensearch.utils.Utils.agg;
import static org.opensearch.sql.opensearch.utils.Utils.avg;
import static org.opensearch.sql.opensearch.utils.Utils.group;
import static org.opensearch.sql.opensearch.utils.Utils.sort;

import com.fasterxml.jackson.databind.ObjectMapper;
import java.util.AbstractMap;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Set;
import lombok.SneakyThrows;
import org.apache.commons.lang3.tuple.Pair;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.DisplayNameGeneration;
import org.junit.jupiter.api.DisplayNameGenerator;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;
import org.opensearch.sql.ast.tree.Sort;
import org.opensearch.sql.data.type.ExprType;
import org.opensearch.sql.expression.DSL;
import org.opensearch.sql.expression.Expression;
import org.opensearch.sql.expression.NamedExpression;
import org.opensearch.sql.expression.aggregation.AvgAggregator;
import org.opensearch.sql.expression.aggregation.CountAggregator;
import org.opensearch.sql.expression.aggregation.NamedAggregator;
import org.opensearch.sql.expression.config.ExpressionConfig;
import org.opensearch.sql.opensearch.storage.serialization.ExpressionSerializer;

@DisplayNameGeneration(DisplayNameGenerator.ReplaceUnderscores.class)
@ExtendWith(MockitoExtension.class)
class AggregationQueryBuilderTest {

  private final DSL dsl = new ExpressionConfig().dsl(new ExpressionConfig().functionRepository());

  @Mock
  private ExpressionSerializer serializer;

  private AggregationQueryBuilder queryBuilder;

  @BeforeEach
  void set_up() {
    queryBuilder = new AggregationQueryBuilder(serializer);
  }

  @Test
  void should_build_composite_aggregation_for_field_reference() {
    assertEquals(
        "{\n"
            + "  \"composite_buckets\" : {\n"
            + "    \"composite\" : {\n"
            + "      \"size\" : 1000,\n"
            + "      \"sources\" : [ {\n"
            + "        \"name\" : {\n"
            + "          \"terms\" : {\n"
            + "            \"field\" : \"name\",\n"
            + "            \"missing_bucket\" : true,\n"
            + "            \"order\" : \"asc\"\n"
            + "          }\n"
            + "        }\n"
            + "      } ]\n"
            + "    },\n"
            + "    \"aggregations\" : {\n"
            + "      \"avg(age)\" : {\n"
            + "        \"avg\" : {\n"
            + "          \"field\" : \"age\"\n"
            + "        }\n"
            + "      }\n"
            + "    }\n"
            + "  }\n"
            + "}",
        buildQuery(
            Arrays.asList(
                named("avg(age)", new AvgAggregator(Arrays.asList(ref("age", INTEGER)), INTEGER))),
            Arrays.asList(named("name", ref("name", STRING)))));
  }

  @Test
  void should_build_composite_aggregation_for_field_reference_with_order() {
    assertEquals(
        "{\n"
            + "  \"composite_buckets\" : {\n"
            + "    \"composite\" : {\n"
            + "      \"size\" : 1000,\n"
            + "      \"sources\" : [ {\n"
            + "        \"name\" : {\n"
            + "          \"terms\" : {\n"
            + "            \"field\" : \"name\",\n"
            + "            \"missing_bucket\" : true,\n"
            + "            \"order\" : \"desc\"\n"
            + "          }\n"
            + "        }\n"
            + "      } ]\n"
            + "    },\n"
            + "    \"aggregations\" : {\n"
            + "      \"avg(age)\" : {\n"
            + "        \"avg\" : {\n"
            + "          \"field\" : \"age\"\n"
            + "        }\n"
            + "      }\n"
            + "    }\n"
            + "  }\n"
            + "}",
        buildQuery(
            Arrays.asList(
                named("avg(age)", new AvgAggregator(Arrays.asList(ref("age", INTEGER)), INTEGER))),
            Arrays.asList(named("name", ref("name", STRING))),
            sort(ref("name", STRING), Sort.SortOption.DEFAULT_DESC)
        ));
  }

  @Test
  void should_build_type_mapping_for_field_reference() {
    assertThat(
        buildTypeMapping(Arrays.asList(
            named("avg(age)", new AvgAggregator(Arrays.asList(ref("age", INTEGER)), INTEGER))),
            Arrays.asList(named("name", ref("name", STRING)))),
        containsInAnyOrder(
            map("avg(age)", INTEGER),
            map("name", STRING)
        ));
  }

  @Test
  void should_build_composite_aggregation_for_field_reference_of_keyword() {
    assertEquals(
        "{\n"
            + "  \"composite_buckets\" : {\n"
            + "    \"composite\" : {\n"
            + "      \"size\" : 1000,\n"
            + "      \"sources\" : [ {\n"
            + "        \"name\" : {\n"
            + "          \"terms\" : {\n"
            + "            \"field\" : \"name.keyword\",\n"
            + "            \"missing_bucket\" : true,\n"
            + "            \"order\" : \"asc\"\n"
            + "          }\n"
            + "        }\n"
            + "      } ]\n"
            + "    },\n"
            + "    \"aggregations\" : {\n"
            + "      \"avg(age)\" : {\n"
            + "        \"avg\" : {\n"
            + "          \"field\" : \"age\"\n"
            + "        }\n"
            + "      }\n"
            + "    }\n"
            + "  }\n"
            + "}",
        buildQuery(
            Arrays.asList(
                named("avg(age)", new AvgAggregator(Arrays.asList(ref("age", INTEGER)), INTEGER))),
            Arrays.asList(named("name", ref("name", OPENSEARCH_TEXT_KEYWORD)))));
  }

  @Test
  void should_build_type_mapping_for_field_reference_of_keyword() {
    assertThat(
        buildTypeMapping(Arrays.asList(
            named("avg(age)", new AvgAggregator(Arrays.asList(ref("age", INTEGER)), INTEGER))),
            Arrays.asList(named("name", ref("name", OPENSEARCH_TEXT_KEYWORD)))),
        containsInAnyOrder(
            map("avg(age)", INTEGER),
            map("name", OPENSEARCH_TEXT_KEYWORD)
        ));
  }

  @Test
  void should_build_composite_aggregation_for_expression() {
    doAnswer(invocation -> {
      Expression expr = invocation.getArgument(0);
      return expr.toString();
    }).when(serializer).serialize(any());
    assertEquals(
        "{\n"
            + "  \"composite_buckets\" : {\n"
            + "    \"composite\" : {\n"
            + "      \"size\" : 1000,\n"
            + "      \"sources\" : [ {\n"
            + "        \"age\" : {\n"
            + "          \"terms\" : {\n"
            + "            \"script\" : {\n"
            + "              \"source\" : \"asin(age)\",\n"
            + "              \"lang\" : \"opensearch_query_expression\"\n"
            + "            },\n"
            + "            \"missing_bucket\" : true,\n"
            + "            \"order\" : \"asc\"\n"
            + "          }\n"
            + "        }\n"
            + "      } ]\n"
            + "    },\n"
            + "    \"aggregations\" : {\n"
            + "      \"avg(balance)\" : {\n"
            + "        \"avg\" : {\n"
            + "          \"script\" : {\n"
            + "            \"source\" : \"abs(balance)\",\n"
            + "            \"lang\" : \"opensearch_query_expression\"\n"
            + "          }\n"
            + "        }\n"
            + "      }\n"
            + "    }\n"
            + "  }\n"
            + "}",
        buildQuery(
            Arrays.asList(
                named("avg(balance)", new AvgAggregator(
                    Arrays.asList(dsl.abs(ref("balance", INTEGER))), INTEGER))),
            Arrays.asList(named("age", dsl.asin(ref("age", INTEGER))))));
  }

  @Test
  void should_build_composite_aggregation_follow_with_order_by_position() {
    assertEquals(
        "{\n"
            + "  \"composite_buckets\" : {\n"
            + "    \"composite\" : {\n"
            + "      \"size\" : 1000,\n"
            + "      \"sources\" : [ {\n"
            + "        \"name\" : {\n"
            + "          \"terms\" : {\n"
            + "            \"field\" : \"name\",\n"
            + "            \"missing_bucket\" : true,\n"
            + "            \"order\" : \"desc\"\n"
            + "          }\n"
            + "        }\n"
            + "      }, {\n"
            + "        \"age\" : {\n"
            + "          \"terms\" : {\n"
            + "            \"field\" : \"age\",\n"
            + "            \"missing_bucket\" : true,\n"
            + "            \"order\" : \"asc\"\n"
            + "          }\n"
            + "        }\n"
            + "      } ]\n"
            + "    },\n"
            + "    \"aggregations\" : {\n"
            + "      \"avg(balance)\" : {\n"
            + "        \"avg\" : {\n"
            + "          \"field\" : \"balance\"\n"
            + "        }\n"
            + "      }\n"
            + "    }\n"
            + "  }\n"
            + "}",
        buildQuery(
            agg(named("avg(balance)", avg(ref("balance", INTEGER), INTEGER))),
            group(named("age", ref("age", INTEGER)), named("name", ref("name", STRING))),
            sort(ref("name", STRING), Sort.SortOption.DEFAULT_DESC,
                ref("age", INTEGER), Sort.SortOption.DEFAULT_ASC)
        ));
  }

  @Test
  void should_build_type_mapping_for_expression() {
    assertThat(
        buildTypeMapping(Arrays.asList(
            named("avg(balance)", new AvgAggregator(
                Arrays.asList(dsl.abs(ref("balance", INTEGER))), INTEGER))),
            Arrays.asList(named("age", dsl.asin(ref("age", INTEGER))))),
        containsInAnyOrder(
            map("avg(balance)", INTEGER),
            map("age", DOUBLE)
        ));
  }

  @Test
  void should_build_aggregation_without_bucket() {
    assertEquals(
        "{\n"
            + "  \"avg(balance)\" : {\n"
            + "    \"avg\" : {\n"
            + "      \"field\" : \"balance\"\n"
            + "    }\n"
            + "  }\n"
            + "}",
        buildQuery(
            Arrays.asList(
                named("avg(balance)", new AvgAggregator(
                    Arrays.asList(ref("balance", INTEGER)), INTEGER))),
            Collections.emptyList()));
  }

  @Test
  void should_build_filter_aggregation() {
    assertEquals(
        "{\n" 
            + "  \"avg(age) filter(where age > 34)\" : {\n"
            + "    \"filter\" : {\n" 
            + "      \"range\" : {\n" 
            + "        \"age\" : {\n" 
            + "          \"from\" : 20,\n" 
            + "          \"to\" : null,\n" 
            + "          \"include_lower\" : false,\n" 
            + "          \"include_upper\" : true,\n" 
            + "          \"boost\" : 1.0\n" 
            + "        }\n" 
            + "      }\n" 
            + "    },\n" 
            + "    \"aggregations\" : {\n" 
            + "      \"avg(age) filter(where age > 34)\" : {\n" 
            + "        \"avg\" : {\n" 
            + "          \"field\" : \"age\"\n" 
            + "        }\n" 
            + "      }\n" 
            + "    }\n" 
            + "  }\n" 
            + "}",
        buildQuery(
            Arrays.asList(named("avg(age) filter(where age > 34)",
                new AvgAggregator(Arrays.asList(ref("age", INTEGER)), INTEGER)
                    .condition(dsl.greater(ref("age", INTEGER), literal(20))))),
            Collections.emptyList()));
  }

  @Test
  void should_build_filter_aggregation_group_by() {
    assertEquals(
        "{\n" 
            + "  \"composite_buckets\" : {\n" 
            + "    \"composite\" : {\n" 
            + "      \"size\" : 1000,\n" 
            + "      \"sources\" : [ {\n" 
            + "        \"gender\" : {\n" 
            + "          \"terms\" : {\n" 
            + "            \"field\" : \"gender\",\n" 
            + "            \"missing_bucket\" : true,\n" 
            + "            \"order\" : \"asc\"\n" 
            + "          }\n" 
            + "        }\n" 
            + "      } ]\n" 
            + "    },\n" 
            + "    \"aggregations\" : {\n" 
            + "      \"avg(age) filter(where age > 34)\" : {\n" 
            + "        \"filter\" : {\n" 
            + "          \"range\" : {\n" 
            + "            \"age\" : {\n" 
            + "              \"from\" : 20,\n" 
            + "              \"to\" : null,\n" 
            + "              \"include_lower\" : false,\n" 
            + "              \"include_upper\" : true,\n" 
            + "              \"boost\" : 1.0\n" 
            + "            }\n" 
            + "          }\n" 
            + "        },\n" 
            + "        \"aggregations\" : {\n" 
            + "          \"avg(age) filter(where age > 34)\" : {\n" 
            + "            \"avg\" : {\n" 
            + "              \"field\" : \"age\"\n" 
            + "            }\n" 
            + "          }\n" 
            + "        }\n" 
            + "      }\n" 
            + "    }\n" 
            + "  }\n" 
            + "}",
        buildQuery(
            Arrays.asList(named("avg(age) filter(where age > 34)",
                new AvgAggregator(Arrays.asList(ref("age", INTEGER)), INTEGER)
                    .condition(dsl.greater(ref("age", INTEGER), literal(20))))),
            Arrays.asList(named(ref("gender", STRING)))));
  }

  @Test
  void should_build_type_mapping_without_bucket() {
    assertThat(
        buildTypeMapping(Arrays.asList(
            named("avg(balance)", new AvgAggregator(
                Arrays.asList(ref("balance", INTEGER)), INTEGER))),
            Collections.emptyList()),
        containsInAnyOrder(
            map("avg(balance)", INTEGER)
        ));
  }

  @Test
  void should_build_histogram() {
    assertEquals(
        "{\n"
            + "  \"SpanExpression(field=age, value=10, unit=NONE)\" : {\n"
            + "    \"histogram\" : {\n"
            + "      \"field\" : \"age\",\n"
            + "      \"interval\" : 10.0,\n"
            + "      \"offset\" : 0.0,\n"
            + "      \"order\" : {\n"
            + "        \"_key\" : \"asc\"\n"
            + "      },\n"
            + "      \"keyed\" : false,\n"
            + "      \"min_doc_count\" : 0\n"
            + "    },\n"
            + "    \"aggregations\" : {\n"
            + "      \"count(a)\" : {\n"
            + "        \"value_count\" : {\n"
            + "          \"field\" : \"a\"\n"
            + "        }\n"
            + "      }\n"
            + "    }\n"
            + "  }\n"
            + "}",
        buildQuery(Arrays.asList(named("count(a)", new CountAggregator(Arrays.asList(ref(
            "a", INTEGER)), INTEGER))),
            Arrays.asList(named(span(ref("age", INTEGER), literal(10), "")))));
  }

  @SneakyThrows
  private String buildQuery(List<NamedAggregator> namedAggregatorList,
                            List<NamedExpression> groupByList) {
    return buildQuery(namedAggregatorList, groupByList, null);
  }

  @SneakyThrows
  private String buildQuery(
      List<NamedAggregator> namedAggregatorList,
      List<NamedExpression> groupByList,
      List<Pair<Sort.SortOption, Expression>> sortList) {
    ObjectMapper objectMapper = new ObjectMapper();
    return objectMapper
        .readTree(
            queryBuilder
                .buildAggregationBuilder(namedAggregatorList, groupByList, sortList)
                .getLeft()
                .get(0)
                .toString())
        .toPrettyString();
  }

  private Set<Map.Entry<String, ExprType>> buildTypeMapping(
      List<NamedAggregator> namedAggregatorList,
      List<NamedExpression> groupByList) {
    return queryBuilder.buildTypeMapping(namedAggregatorList, groupByList).entrySet();
  }

  private Map.Entry<String, ExprType> map(String name, ExprType type) {
    return new AbstractMap.SimpleEntry<String, ExprType>(name, type);
  }
}
