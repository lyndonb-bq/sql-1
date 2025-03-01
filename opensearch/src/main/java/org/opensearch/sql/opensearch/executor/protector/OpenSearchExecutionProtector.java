/*
 * Copyright OpenSearch Contributors
 * SPDX-License-Identifier: Apache-2.0
 */


package org.opensearch.sql.opensearch.executor.protector;

import lombok.RequiredArgsConstructor;
import org.opensearch.sql.monitor.ResourceMonitor;
import org.opensearch.sql.planner.physical.AggregationOperator;
import org.opensearch.sql.planner.physical.DedupeOperator;
import org.opensearch.sql.planner.physical.EvalOperator;
import org.opensearch.sql.planner.physical.FilterOperator;
import org.opensearch.sql.planner.physical.LimitOperator;
import org.opensearch.sql.planner.physical.PhysicalPlan;
import org.opensearch.sql.planner.physical.ProjectOperator;
import org.opensearch.sql.planner.physical.RareTopNOperator;
import org.opensearch.sql.planner.physical.RemoveOperator;
import org.opensearch.sql.planner.physical.RenameOperator;
import org.opensearch.sql.planner.physical.SortOperator;
import org.opensearch.sql.planner.physical.ValuesOperator;
import org.opensearch.sql.planner.physical.WindowOperator;
import org.opensearch.sql.storage.TableScanOperator;

/**
 * OpenSearch Execution Protector.
 */
@RequiredArgsConstructor
public class OpenSearchExecutionProtector extends ExecutionProtector {

  /**
   * OpenSearch resource monitor.
   */
  private final ResourceMonitor resourceMonitor;

  public PhysicalPlan protect(PhysicalPlan physicalPlan) {
    return physicalPlan.accept(this, null);
  }

  @Override
  public PhysicalPlan visitFilter(FilterOperator node, Object context) {
    return new FilterOperator(visitInput(node.getInput(), context), node.getConditions());
  }

  @Override
  public PhysicalPlan visitAggregation(AggregationOperator node, Object context) {
    return new AggregationOperator(visitInput(node.getInput(), context), node.getAggregatorList(),
        node.getGroupByExprList());
  }

  @Override
  public PhysicalPlan visitRareTopN(RareTopNOperator node, Object context) {
    return new RareTopNOperator(visitInput(node.getInput(), context), node.getCommandType(),
        node.getNoOfResults(), node.getFieldExprList(), node.getGroupByExprList());
  }

  @Override
  public PhysicalPlan visitRename(RenameOperator node, Object context) {
    return new RenameOperator(visitInput(node.getInput(), context), node.getMapping());
  }

  /**
   * Decorate with {@link ResourceMonitorPlan}.
   */
  @Override
  public PhysicalPlan visitTableScan(TableScanOperator node, Object context) {
    return doProtect(node);
  }

  @Override
  public PhysicalPlan visitProject(ProjectOperator node, Object context) {
    return new ProjectOperator(visitInput(node.getInput(), context), node.getProjectList());
  }

  @Override
  public PhysicalPlan visitRemove(RemoveOperator node, Object context) {
    return new RemoveOperator(visitInput(node.getInput(), context), node.getRemoveList());
  }

  @Override
  public PhysicalPlan visitEval(EvalOperator node, Object context) {
    return new EvalOperator(visitInput(node.getInput(), context), node.getExpressionList());
  }

  @Override
  public PhysicalPlan visitDedupe(DedupeOperator node, Object context) {
    return new DedupeOperator(visitInput(node.getInput(), context), node.getDedupeList(),
        node.getAllowedDuplication(), node.getKeepEmpty(), node.getConsecutive());
  }

  @Override
  public PhysicalPlan visitWindow(WindowOperator node, Object context) {
    return new WindowOperator(
        doProtect(visitInput(node.getInput(), context)),
        node.getWindowFunction(),
        node.getWindowDefinition());
  }

  /**
   * Decorate with {@link ResourceMonitorPlan}.
   */
  @Override
  public PhysicalPlan visitSort(SortOperator node, Object context) {
    return doProtect(
        new SortOperator(
            visitInput(node.getInput(), context),
            node.getSortList()));
  }

  /**
   * Values are a sequence of rows of literal value in memory
   * which doesn't need memory protection.
   */
  @Override
  public PhysicalPlan visitValues(ValuesOperator node, Object context) {
    return node;
  }

  @Override
  public PhysicalPlan visitLimit(LimitOperator node, Object context) {
    return new LimitOperator(
        visitInput(node.getInput(), context),
        node.getLimit(),
        node.getOffset());
  }

  PhysicalPlan visitInput(PhysicalPlan node, Object context) {
    if (null == node) {
      return node;
    } else {
      return node.accept(this, context);
    }
  }

  private PhysicalPlan doProtect(PhysicalPlan node) {
    if (isProtected(node)) {
      return node;
    }
    return new ResourceMonitorPlan(node, resourceMonitor);
  }

  private boolean isProtected(PhysicalPlan node) {
    return (node instanceof ResourceMonitorPlan);
  }

}
