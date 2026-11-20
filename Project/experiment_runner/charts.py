"""PNG chart rendering for structured experiment comparisons."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Optional, Tuple

from .comparison import ComparisonReport, MetricComparison, MetricSelector

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:  # pragma: no cover - exercised only when Pillow is absent
    Image = None
    ImageDraw = None
    ImageFont = None


@dataclass(frozen=True)
class ChartBar:
    run_id: str
    value: float
    percent_change: Optional[float]
    color: str


class ComparisonChartRenderer:
    """Render comparison metrics as report-ready PNG images."""

    WIDTH = 1600
    HEIGHT = 900
    MARGIN = 54
    GAP = 24
    BACKGROUND = "#F5F7FA"
    PANEL = "#FFFFFF"
    PANEL_BORDER = "#DCE3EC"
    TITLE = "#102033"
    TEXT = "#475569"
    MUTED = "#64748B"
    TRACK = "#E2E8F0"
    BASELINE = "#94A3B8"
    CANDIDATE_COLORS = ("#2563EB", "#14B8A6", "#F59E0B", "#8B5CF6", "#EF4444")

    def __init__(self, output_dir: Path) -> None:
        self.output_dir = Path(output_dir)

    def render(self, report: ComparisonReport) -> Dict[str, Path]:
        if Image is None or ImageDraw is None or ImageFont is None:
            raise RuntimeError("Pillow is required to render comparison charts")

        self.output_dir.mkdir(parents=True, exist_ok=True)
        overview = self.output_dir / "comparison_overview.png"
        quality = self.output_dir / "comparison_prefetch_quality.png"
        sources = self.output_dir / "comparison_per_source.png"

        self._render_dashboard(
            report,
            overview,
            title="ChampSim Comparison Overview",
            panels=(
                ("IPC", MetricSelector("cpu0", "ipc")),
                (
                    "L2C LOAD Miss",
                    MetricSelector("cpu0_L2C", "cache_miss", dimensions=(("access_type", "LOAD"),)),
                ),
                ("Useful Prefetches", MetricSelector("cpu0_L2C", "prefetch_useful")),
                ("Prefetch Accuracy", MetricSelector("cpu0_L2C", "prefetch_accuracy")),
            ),
        )
        self._render_dashboard(
            report,
            quality,
            title="Prefetch Quality and Timeliness",
            panels=(
                ("Issued Prefetches", MetricSelector("cpu0_L2C", "prefetch_issued")),
                ("Prefetch Coverage", MetricSelector("cpu0_L2C", "prefetch_coverage")),
                ("Timely Rate", MetricSelector("cpu0_L2C", "prefetch_timely_rate")),
                ("Mean Lead Time (cycles)", MetricSelector("cpu0_L2C", "prefetch_mean_lead_time")),
            ),
        )
        self._render_dashboard(
            report,
            sources,
            title="Per-Source Prefetch Contribution",
            panels=(
                (
                    "Source 0: Useful",
                    MetricSelector(
                        "cpu0_L2C",
                        "prefetch_source_useful",
                        dimensions=(("prefetcher_id", "0"),),
                    ),
                ),
                (
                    "Source 1: Useful",
                    MetricSelector(
                        "cpu0_L2C",
                        "prefetch_source_useful",
                        dimensions=(("prefetcher_id", "1"),),
                    ),
                ),
                (
                    "Source 0: Accuracy",
                    MetricSelector(
                        "cpu0_L2C",
                        "prefetch_source_accuracy",
                        dimensions=(("prefetcher_id", "0"),),
                    ),
                ),
                (
                    "Source 1: Accuracy",
                    MetricSelector(
                        "cpu0_L2C",
                        "prefetch_source_accuracy",
                        dimensions=(("prefetcher_id", "1"),),
                    ),
                ),
            ),
        )
        return {
            "overview": overview,
            "prefetch_quality": quality,
            "per_source": sources,
        }

    def _render_dashboard(
        self,
        report: ComparisonReport,
        output_path: Path,
        title: str,
        panels: Tuple[Tuple[str, MetricSelector], ...],
    ) -> None:
        image = Image.new("RGB", (self.WIDTH, self.HEIGHT), self.BACKGROUND)
        draw = ImageDraw.Draw(image)
        title_font = self._font(34, bold=True)
        subtitle_font = self._font(16)
        footer_font = self._font(14)

        draw.text((self.MARGIN, 30), title, font=title_font, fill=self.TITLE)
        draw.text(
            (self.MARGIN, 76),
            f"Baseline: {report.baseline_run_id}  |  candidates: "
            + ", ".join(candidate.candidate_run_id for candidate in report.candidates),
            font=subtitle_font,
            fill=self.MUTED,
        )

        content_top = 118
        content_bottom = 820
        panel_width = (self.WIDTH - 2 * self.MARGIN - self.GAP) // 2
        panel_height = (content_bottom - content_top - self.GAP) // 2
        for index, (panel_title, selector) in enumerate(panels):
            row, column = divmod(index, 2)
            left = self.MARGIN + column * (panel_width + self.GAP)
            top = content_top + row * (panel_height + self.GAP)
            self._draw_panel(
                draw,
                (left, top, left + panel_width, top + panel_height),
                panel_title,
                report,
                selector,
            )

        draw.text(
            (self.MARGIN, 856),
            "Generated from structured metrics. Bars show absolute values; candidate labels show relative change from baseline.",
            font=footer_font,
            fill=self.MUTED,
        )
        image.save(output_path, format="PNG", optimize=True)

    def _draw_panel(
        self,
        draw,
        box: Tuple[int, int, int, int],
        title: str,
        report: ComparisonReport,
        selector: MetricSelector,
    ) -> None:
        left, top, right, bottom = box
        draw.rounded_rectangle(box, radius=18, fill=self.PANEL, outline=self.PANEL_BORDER, width=2)
        draw.text((left + 24, top + 20), title, font=self._font(21, bold=True), fill=self.TITLE)

        bars = self._collect_bars(report, selector)
        if not bars:
            draw.text(
                (left + 24, top + 76),
                "Metric unavailable in this comparison",
                font=self._font(16),
                fill=self.MUTED,
            )
            return

        percent_mode = self._is_percentage(selector.metric_id, bars)
        max_width = right - left - 480
        label_width = 250
        bar_left = left + 286
        bar_top = top + 90
        row_height = 52
        bar_height = 28
        scale_values = [abs(bar.value) * (100.0 if percent_mode else 1.0) for bar in bars]
        maximum = max(scale_values) if scale_values else 0.0

        for index, bar in enumerate(bars):
            y = bar_top + index * row_height
            if y + bar_height > bottom - 24:
                break
            color = bar.color
            label = self._truncate(bar.run_id, 28)
            draw.rounded_rectangle(
                (left + 24, y, left + 24 + label_width, y + bar_height),
                radius=8,
                fill=color,
            )
            draw.text(
                (left + 34, y + 5),
                label,
                font=self._font(14),
                fill="#FFFFFF",
            )

            draw.rounded_rectangle(
                (bar_left, y, bar_left + max_width, y + bar_height),
                radius=8,
                fill=self.TRACK,
            )
            if maximum > 0:
                width = max(4, int(max_width * scale_values[index] / maximum))
                draw.rounded_rectangle(
                    (bar_left, y, bar_left + width, y + bar_height),
                    radius=8,
                    fill=color,
                )

            value_text = self._format_value(bar.value, percent_mode)
            if bar.percent_change is not None and index > 0:
                value_text += f"  ({bar.percent_change:+.2f}%)"
            draw.text(
                (bar_left + max_width + 12, y + 5),
                value_text,
                font=self._font(14),
                fill=self.TEXT,
            )

    def _collect_bars(
        self,
        report: ComparisonReport,
        selector: MetricSelector,
    ) -> Tuple[ChartBar, ...]:
        candidates = []
        for run_comparison in report.candidates:
            metric = next(
                (item for item in run_comparison.metrics if selector.matches(item.key)),
                None,
            )
            if metric is not None:
                candidates.append(metric)
        if not candidates:
            return ()

        baseline = candidates[0].baseline
        bars = [
            ChartBar(
                run_id=report.baseline_run_id,
                value=baseline.value,
                percent_change=None,
                color=self.BASELINE,
            )
        ]
        for index, metric in enumerate(candidates):
            bars.append(
                ChartBar(
                    run_id=report.candidates[index].candidate_run_id,
                    value=metric.candidate.value,
                    percent_change=metric.percent_change,
                    color=self.CANDIDATE_COLORS[index % len(self.CANDIDATE_COLORS)],
                )
            )
        return tuple(bars)

    @staticmethod
    def _is_percentage(metric_id: str, bars: Tuple[ChartBar, ...]) -> bool:
        return any(token in metric_id for token in ("rate", "accuracy", "coverage"))

    def _font(self, size: int, bold: bool = False):
        candidates = (
            "C:/Windows/Fonts/arialbd.ttf" if bold else "C:/Windows/Fonts/arial.ttf",
            "DejaVuSans-Bold.ttf" if bold else "DejaVuSans.ttf",
        )
        for name in candidates:
            try:
                return ImageFont.truetype(name, size=size)
            except OSError:
                continue
        return ImageFont.load_default()

    @staticmethod
    def _truncate(text: str, limit: int) -> str:
        return text if len(text) <= limit else text[: limit - 3] + "..."

    @staticmethod
    def _format_value(value: float, percentage: bool) -> str:
        if percentage:
            return f"{value * 100.0:.2f}%"
        if abs(value - round(value)) < 1e-9:
            return f"{int(round(value)):,}"
        if abs(value) < 10:
            return f"{value:.6f}"
        return f"{value:,.2f}"


