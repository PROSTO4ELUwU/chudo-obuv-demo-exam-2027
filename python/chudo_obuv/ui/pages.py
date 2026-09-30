"""Базовая страница и общий контекст приложения."""

from dataclasses import dataclass, field
from typing import Protocol

from PySide6.QtWidgets import QWidget

from chudo_obuv.models import GUEST, User
from chudo_obuv.order_draft import OrderDraft
from chudo_obuv.repositories import OrderRepository, ProductRepository, UserRepository


class Navigator(Protocol):
    """Переходы между страницами; реализуются главным окном."""

    def show_catalog(self) -> None: ...

    def go_back(self) -> None: ...


@dataclass
class AppContext:
    """Всё, что нужно страницам: доступ к данным, пользователь и его заказ."""

    users: UserRepository
    products: ProductRepository
    orders: OrderRepository
    user: User = GUEST
    draft: OrderDraft = field(default_factory=OrderDraft)
    navigator: Navigator | None = None


class Page(QWidget):
    """Страница, которую главное окно показывает в стеке навигации."""

    title = ""

    def __init__(self, context: AppContext) -> None:
        super().__init__()
        self.context = context

    @property
    def navigator(self) -> Navigator:
        """Объект, выполняющий переходы между страницами."""
        return self.context.navigator

    def on_activated(self) -> None:
        """Вызывается при каждом показе страницы, в том числе при возврате «Назад»."""
